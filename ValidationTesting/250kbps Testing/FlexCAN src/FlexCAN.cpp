// -------------------------------------------------------------
// a simple Arduino Teensy 3.1/3.2/3.5/3.6 CAN driver
// by teachop
// dual CAN support for MK66FX1M0 and updates for MK64FX512 by Pawelsky
// Interrupt driven Rx/Tx with buffers, object oriented callbacks by Collin Kidder
// RTR related code by H4nky84
// Statistics collection, timestamp and code clean-up my mdapoz
//
//
#include "FlexCAN.h"
#include "kinetis_flexcan.h"
#include "error.h" //from Linux SocketCAN
#include "EEPROM.h" // to read previous bitrate index from EEPROM


#define FLEXCANb_MCR(b)                   (*(vuint32_t*)(b))
#define FLEXCANb_CTRL1(b)                 (*(vuint32_t*)(b+4))
#define FLEXCANb_RXMGMASK(b)              (*(vuint32_t*)(b+0x10))
#define FLEXCANb_ECR(b)                   (*(vuint32_t*)(b+0x1C))
#define FLEXCANb_IFLAG1(b)                (*(vuint32_t*)(b+0x30))
#define FLEXCANb_IMASK1(b)                (*(vuint32_t*)(b+0x28))

#define FLEXCANb_RXFGMASK(b)              (*(vuint32_t*)(b+0x48))
#define FLEXCANb_MBn_CS(b, n)             (*(vuint32_t*)(b+0x80+n*0x10))
#define FLEXCANb_MBn_ID(b, n)             (*(vuint32_t*)(b+0x84+n*0x10))
#define FLEXCANb_MBn_WORD0(b, n)          (*(vuint32_t*)(b+0x88+n*0x10))
#define FLEXCANb_MBn_WORD1(b, n)          (*(vuint32_t*)(b+0x8C+n*0x10))
#define FLEXCANb_IDFLT_TAB(b, n)          (*(vuint32_t*)(b+0xE0+(n*4)))
#define FLEXCANb_MB_MASK(b, n)            (*(vuint32_t*)(b+0x880+(n*4)))
#define FLEXCANb_ESR1(b)                  (*(vuint32_t*)(b+0x20)) 

#if defined(__MK66FX1M0__)
  #define INCLUDE_FLEXCAN_CAN1
#endif


// Supported FlexCAN interfaces

FlexCAN Can0 (0);
#if defined(INCLUDE_FLEXCAN_CAN1)
FlexCAN Can1 (1);
#endif

// default mask to apply to all mailboxes

CAN_filter_t FlexCAN::defaultMask;

// Some of these are complete guesses. Only really 8 and 16 have been validated.
// You have been warned. But, there aren't too many options for some of these

uint8_t bitTimingTable[21][3] =
{
    // prop, seg1, seg2 (4 + prop + seg1 + seg2, seg2 must be at least 1)
    // No value can go over 7 here.
    {0,0,1}, //5
    {1,0,1}, //6
    {1,1,1}, //7
    {2,1,1}, //8
    {2,2,1}, //9
    {2,3,1}, //10
    {2,3,2}, //11
    {2,4,2}, //12
    {2,5,2}, //13
    {2,5,3}, //14
    {2,6,3}, //15
    {2,7,3}, //16
    {2,7,4}, //17
    {3,7,4}, //18
    {3,7,5}, //19
    {4,7,5}, //20
    {4,7,6}, //21
    {5,7,6}, //22
    {6,7,6}, //23
    {6,7,7}, //24
    {7,7,7}, //25
};

 /*
  * \brief Initialize the FlexCAN driver class
  *
  * \param id - CAN bus interface selection
  *
  * \retval none
  *
  */

FlexCAN::FlexCAN (uint8_t id)
{
  
  can_channel = id;
  flexcanBase = FLEXCAN0_BASE;
  eeprom_RATE_INDEX_ADDR = EEPROM_BIT_RATE_INDEX_ADDR;

  numTxMailboxes = NUM_TX_MAILBOXES;

#if defined (INCLUDE_FLEXCAN_CAN1)
  if (id > 0)  {
    flexcanBase = FLEXCAN1_BASE;
    eeprom_RATE_INDEX_ADDR = EEPROM_BIT_RATE_INDEX_ADDR + 1;
  }
#endif

  // Default mask is allow everything

  defaultMask.flags.remote = 0;
  defaultMask.flags.extended = 0;
  defaultMask.id = 0;

  // set up the transmit and receive ring buffers

  initRingBuffer (txRing, tx_buffer, SIZE_TX_BUFFER);
  initRingBuffer (rxRing, rx_buffer, SIZE_RX_BUFFER);

  // clear any listeners for received packets

  for (size_t i = 0; i < SIZE_LISTENERS; i++) {
    listener[i] = NULL;
  }

  // clear statistics counts
  clearStats ();
}


 /*
  * \brief Bring the hardware into freeze which drops it off the CAN bus
  *
  * \param none
  *
  * \retval none
  *
  */

void FlexCAN::end (void)
{
  // enter freeze mode
  freezeMode(true);
}

 /*
  * \brief Initializes the CAN bus to the given settings
  *
  * \param baud - Set the baud rate of the bus. 
  *               Only certain values are valid 50000, 100000, 125000, 250000, 500000, 1000000
  *               If a 0 is used, then an autobaud technique is used.
  * \param mask - A default mask to use for all mailbox masks. Optional.
  * \param txAlt - 1 to enable alternate Tx pin (where available)
  * \param rxAlt - 1 to enable alternate Rx pin (where available)
  *
  * \retval none
  *
  */

void FlexCAN::begin (uint32_t baud, const CAN_filter_t &mask, uint8_t txAlt, uint8_t rxAlt)
{
    if (baud == 0) autobaud = true;
    else autobaud = false;
    // set up the pins

    if (flexcanBase == FLEXCAN0_BASE) {
        dbg_println ("Begin setup of CAN0");

#if defined(__MK66FX1M0__) || defined(__MK64FX512__)
        //  3=PTA12=CAN0_TX,  4=PTA13=CAN0_RX (default)
        // 29=PTB18=CAN0_TX, 30=PTB19=CAN0_RX (alternative)

        if (txAlt == 1)
            CORE_PIN29_CONFIG = PORT_PCR_MUX(2);
        else
            CORE_PIN3_CONFIG = PORT_PCR_MUX(2);

        // | PORT_PCR_PE | PORT_PCR_PS;

        if (rxAlt == 1)
            CORE_PIN30_CONFIG = PORT_PCR_MUX(2);
        else
            CORE_PIN4_CONFIG = PORT_PCR_MUX(2);
#else
        //  3=PTA12=CAN0_TX,  4=PTA13=CAN0_RX (default)
        // 32=PTB18=CAN0_TX, 25=PTB19=CAN0_RX (alternative)

        if (txAlt == 1)
            CORE_PIN32_CONFIG = PORT_PCR_MUX(2);
        else
            CORE_PIN3_CONFIG = PORT_PCR_MUX(2);

        // | PORT_PCR_PE | PORT_PCR_PS;

        if (rxAlt == 1)
            CORE_PIN25_CONFIG = PORT_PCR_MUX(2);
        else
            CORE_PIN4_CONFIG = PORT_PCR_MUX(2);
#endif
    }
#if defined(INCLUDE_FLEXCAN_CAN1)
    else if (flexcanBase == FLEXCAN1_BASE) {
        dbg_println("Begin setup of CAN1");

        // 33=PTE24=CAN1_TX, 34=PTE25=CAN1_RX (default)
        // NOTE: Alternative CAN1 pins are not broken out on Teensy 3.6

        CORE_PIN33_CONFIG = PORT_PCR_MUX(2);
        CORE_PIN34_CONFIG = PORT_PCR_MUX(2);// | PORT_PCR_PE | PORT_PCR_PS;
    }
#endif

    // select clock source 16MHz xtal

    OSC0_CR |= OSC_ERCLKEN;

    if (flexcanBase == FLEXCAN0_BASE) {
        SIM_SCGC6 |=  SIM_SCGC6_FLEXCAN0;
#if defined(INCLUDE_FLEXCAN_CAN1)
    } else if (flexcanBase == FLEXCAN1_BASE) {
        SIM_SCGC3 |=  SIM_SCGC3_FLEXCAN1;
#endif
    }

    FLEXCANb_CTRL1(flexcanBase) &= ~FLEXCAN_CTRL_CLK_SRC;

    // enable CAN

    FLEXCANb_MCR (flexcanBase) |=  FLEXCAN_MCR_FRZ;
    FLEXCANb_MCR (flexcanBase) &= ~FLEXCAN_MCR_MDIS;

    while (FLEXCANb_MCR(flexcanBase) & FLEXCAN_MCR_LPM_ACK);

    // soft reset
    FLEXCANb_MCR (flexcanBase) ^=  FLEXCAN_MCR_SOFT_RST;
    while (FLEXCANb_MCR (flexcanBase) & FLEXCAN_MCR_SOFT_RST);

    // wait for freeze ack
    while (!(FLEXCANb_MCR(flexcanBase) & FLEXCAN_MCR_FRZ_ACK));

    // enable warning iterrupts
    FLEXCANb_MCR (flexcanBase) |= FLEXCAN_MCR_WRN_EN;

    // disable self-reception
    FLEXCANb_MCR (flexcanBase) |= FLEXCAN_MCR_SRX_DIS;
    
    // Turn on Error Interrupts
    FLEXCANb_CTRL1 (flexcanBase) |= FLEXCAN_CTRL_ERR_MSK;
    FLEXCANb_CTRL1 (flexcanBase) |= FLEXCAN_CTRL_RWRN_MSK;
    FLEXCANb_CTRL1 (flexcanBase) |= FLEXCAN_CTRL_TWRN_MSK;

    
    // Setup interrupt vectors
#if defined(__MK20DX256__)
    NVIC_SET_PRIORITY (IRQ_CAN_MESSAGE, IRQ_PRIORITY);
    NVIC_ENABLE_IRQ (IRQ_CAN_MESSAGE);
#elif defined(__MK64FX512__)
    NVIC_SET_PRIORITY (IRQ_CAN0_MESSAGE, IRQ_PRIORITY);
    NVIC_ENABLE_IRQ (IRQ_CAN0_MESSAGE);
#elif defined(__MK66FX1M0__)
    if (flexcanBase == FLEXCAN0_BASE) {
        NVIC_SET_PRIORITY (IRQ_CAN0_MESSAGE, IRQ_PRIORITY);
        NVIC_ENABLE_IRQ (IRQ_CAN0_MESSAGE);
        
        NVIC_SET_PRIORITY (IRQ_CAN0_ERROR, IRQ_LOW_PRIORITY);
        NVIC_ENABLE_IRQ (IRQ_CAN0_ERROR);

        NVIC_SET_PRIORITY (IRQ_CAN0_RX_WARN, IRQ_LOW_PRIORITY+1);
        NVIC_ENABLE_IRQ (IRQ_CAN0_RX_WARN);

        NVIC_SET_PRIORITY (IRQ_CAN0_TX_WARN, IRQ_LOW_PRIORITY+2);
        NVIC_ENABLE_IRQ (IRQ_CAN0_TX_WARN);

        NVIC_SET_PRIORITY (IRQ_CAN0_WAKEUP, IRQ_LOW_PRIORITY+3);
        NVIC_ENABLE_IRQ (IRQ_CAN0_WAKEUP);

        NVIC_SET_PRIORITY (IRQ_CAN0_BUS_OFF, IRQ_LOW_PRIORITY+4);
        NVIC_ENABLE_IRQ (IRQ_CAN0_BUS_OFF);
    } else {
        NVIC_SET_PRIORITY (IRQ_CAN1_MESSAGE, IRQ_PRIORITY);
        NVIC_ENABLE_IRQ (IRQ_CAN1_MESSAGE);

        NVIC_SET_PRIORITY (IRQ_CAN1_ERROR, IRQ_LOW_PRIORITY);
        NVIC_ENABLE_IRQ (IRQ_CAN1_ERROR);
        
        NVIC_SET_PRIORITY (IRQ_CAN1_RX_WARN, IRQ_LOW_PRIORITY+1);
        NVIC_ENABLE_IRQ (IRQ_CAN1_RX_WARN);

        NVIC_SET_PRIORITY (IRQ_CAN1_TX_WARN, IRQ_LOW_PRIORITY+2);
        NVIC_ENABLE_IRQ (IRQ_CAN1_TX_WARN);

        NVIC_SET_PRIORITY (IRQ_CAN1_WAKEUP, IRQ_LOW_PRIORITY+3);
        NVIC_ENABLE_IRQ (IRQ_CAN1_WAKEUP);

        NVIC_SET_PRIORITY (IRQ_CAN1_BUS_OFF, IRQ_LOW_PRIORITY+4);
        NVIC_ENABLE_IRQ (IRQ_CAN1_BUS_OFF);
    }
#endif
    // enable per-mailbox filtering
    FLEXCANb_MCR(flexcanBase) |= FLEXCAN_MCR_IRMQ;

    // now have to set mask and filter for all the Rx mailboxes or they won't receive anything by default.
    CAN_filter_t allPassFilter;
    allPassFilter.ext=0;
    for (uint8_t c = 0; c < 6; c++) {
        setMask (0, c);
        setFilter (allPassFilter, c);
    }
    // be sure to include some filters to enable extended ids
    allPassFilter.ext=1;
    for (uint8_t c = 6; c < NUM_MAILBOXES - numTxMailboxes; c++) {
        setMask (0, c);
        setFilter (allPassFilter, c);
    }

    setNumTxBoxes (numTxMailboxes);
    
    // enable interrupt masks for all 16 mailboxes
    FLEXCANb_IMASK1 (flexcanBase) = 0xFFFF;

    if (autobaud) {
      baud = get_baud_rate();
      Serial.print ("Baud rate for Can");
      Serial.print (can_channel);
      Serial.print (" set to ");
      Serial.println (baud);
    }
    else set_baud_rate(baud);

    dbg_println ("FlexCAN initialized properly");
}

/*
 * \brief Set the error reporting switch.
 *        Error messages follow the Linux SocketCAN format
 *
 * \param mode - set report_error?
 *
 * \retval None.
 *
 */
void FlexCAN::setReportErrors (bool mode)
{
  report_errors = mode;
}

/*
 * \brief Self Reception Disable
 * This bit defines whether FlexCAN is allowed to receive frames transmitted by itself. If this bit is asserted,
 * frames transmitted by the module will not be stored in any MB, regardless if the MB is programmed with
 * an ID that matches the transmitted frame, and no interrupt flag or interrupt signal will be generated due to
 * the frame reception. This bit can be written only in Freeze mode because it is blocked by hardware in
 * other modes.
 * 0 Self reception enabled.
 * 1 Self reception disabled.
 *
 * \param mode - set loop only mode?
 *
 * \retval None.
 *
 */
void FlexCAN::setSelfReception (bool mode)
{
  
  freezeMode(true);  
  if (mode)
    FLEXCANb_MCR (flexcanBase) &= ~FLEXCAN_MCR_SRX_DIS;
  else
    FLEXCANb_MCR (flexcanBase) |= FLEXCAN_MCR_SRX_DIS;
  freezeMode(false);
}


/*
 *
 */
uint32_t FlexCAN::get_baud_rate(void)
{
  // empty the ring buffer
  CAN_message_t rxmsg;
  while (read(rxmsg));
  bool previous_listen_only_mode = FLEXCANb_CTRL1(flexcanBase) & FLEXCAN_CTRL_LOM;
  // turn off error reporting
  report_errors = false;

  // set listen only mode so CAN bus is not influenced by the controller
  setListenOnly(true);

  // lookup what the value of the bitrate was before
  uint8_t initial_baud_rate_index = EEPROM.read(eeprom_RATE_INDEX_ADDR);
  if (initial_baud_rate_index >= NUM_BAUD_RATES) initial_baud_rate_index = 0;
  baud_rate_index = initial_baud_rate_index;

  // Setup a timer for an overall baudrate detection timeout
  uint32_t routine_start_time = millis();
  while ((millis() - routine_start_time) < (AUTOBAUD_TIMEOUT * NUM_BAUD_RATES))
  {
    // Set the bit rate
    dbg_print ("Looking for Messages at ");
    dbg_print (baud_rates[baud_rate_index]);
    dbg_println (" bps.");

    set_baud_rate(baud_rates[baud_rate_index]);
    
    //reset error counters
    freezeMode(true);
    FLEXCANb_ECR(flexcanBase) = 0;
    freezeMode(false);
    // read the RECEIVE Error counter
    uint8_t previousREC = readREC();

    // Start a timout for wait for a CAN message to arrive.
    uint32_t frame_start_time = millis();
    while ((millis() - frame_start_time) < AUTOBAUD_TIMEOUT)
    {
      //Check to see if a message was processed into the ring buffer
      if (available())
      {
        // A message was successfully received, 
        if (initial_baud_rate_index != baud_rate_index)
        {
          EEPROM.update(eeprom_RATE_INDEX_ADDR,baud_rate_index);
        }
        setListenOnly(false);
        return baud_rates[baud_rate_index];
      }
      else
      {
        uint8_t currentREC = readREC();
        if ((currentREC - previousREC) > 0) break;
      }
    }
    baud_rate_index++;
    if (baud_rate_index >= NUM_BAUD_RATES) baud_rate_index = 0;
  }

  // Set the previous known rate after an overall timout. 
  set_baud_rate(baud_rates[initial_baud_rate_index]);
  setListenOnly(previous_listen_only_mode);
  return baud_rates[initial_baud_rate_index];
}

/*

 */
bool FlexCAN::set_baud_rate(uint32_t baud)
{
    /*
      now using a system that tries to automatically generate a viable baud setting.
      Bear these things in mind:
      - The master clock is 16Mhz
      - You can freely divide it by anything from 1 to 256
      - There is always a start bit (+1)
      - The rest (prop, seg1, seg2) are specified 1 less than their actual value (aka +1)
      - This gives the low end bit timing as 5 (1 + 1 + 2 + 1) and the high end 25 (1 + 8 + 8 + 8)
      A worked example: 16Mhz clock, divisor = 19+1, bit values add up to 16 = 16Mhz / 20 / 16 = 50k baud
    */

    // have to find a divisor that ends up as close to the target baud as possible while keeping the end result between 5 and 25

    uint32_t divisor = 0;
    uint32_t bestDivisor = 0;
    uint32_t result = 16000000 / baud / (divisor + 1);
    int error = baud - (16000000 / (result * (divisor + 1)));
    int bestError = error;

    while (result > 5) {
        divisor++;
        result = 16000000 / baud / (divisor + 1);

        if (result <= 25) {
            error = baud - (16000000 / (result * (divisor + 1)));

            if (error < 0)
                error *= -1;

            // if this error is better than we've ever seen then use it - it's the best option

            if (error < bestError) {
                bestError = error;
                bestDivisor = divisor;
            }

            // If this is equal to a previously good option then
            // switch to it but only if the bit time result was in the middle of the range
            // this biases the output to use the middle of the range all things being equal
            // Otherwise it might try to use a higher divisor and smaller values for prop, seg1, seg2
            // and that's not necessarily the best idea.

            if ((error == bestError) && (result > 11) && (result < 19)) {
                bestError = error;
                bestDivisor = divisor;
            }
        }
    }

    divisor = bestDivisor;
    result = 16000000 / baud / (divisor + 1);

    if ((result < 5) || (result > 25) || (bestError > 300)) {
        Serial.println ("Abort in CAN begin. Couldn't find a suitable baud config!");
        return false;
    }

    result -= 5; // the bitTimingTable is offset by 5 since there was no reason to store bit timings for invalid numbers
    uint8_t propSeg = bitTimingTable[result][0];
    uint8_t pSeg1   = bitTimingTable[result][1];
    uint8_t pSeg2   = bitTimingTable[result][2];

    // baud rate debug information
    // dbg_println ("Bit time values:");
    // dbg_print ("Prop = ");
    // dbg_println (propSeg + 1);
    // dbg_print ("Seg1 = ");
    // dbg_println (pSeg1 + 1);
    // dbg_print ("Seg2 = ");
    // dbg_println (pSeg2 + 1);
    // dbg_print ("Divisor = ");
    // dbg_println (divisor + 1);

    freezeMode(true);
    FLEXCANb_CTRL1 (flexcanBase) = (FLEXCAN_CTRL_PROPSEG(propSeg) | FLEXCAN_CTRL_RJW(1) | FLEXCAN_CTRL_ERR_MSK |
                                    FLEXCAN_CTRL_PSEG1(pSeg1) | FLEXCAN_CTRL_PSEG2(pSeg2) | FLEXCAN_CTRL_PRESDIV(divisor));
    freezeMode(false);
    return true;
}

/*
 * \brief Set freeze mode for FlexCAN on or off.
 *
 * \param mode - set freeze mode?
 *
 * \retval None.
 *
 */
void FlexCAN::freezeMode(bool mode){
  if (mode){
    // enter freeze mode if not already there
    if (!(FLEXCANb_MCR(flexcanBase) & FLEXCAN_MCR_FRZ_ACK)) {
       FLEXCANb_MCR(flexcanBase) |= FLEXCAN_MCR_FRZ;
       // stop the CAN
       FLEXCANb_MCR(flexcanBase) |= FLEXCAN_MCR_HALT;
       while (!(FLEXCANb_MCR(flexcanBase) & FLEXCAN_MCR_FRZ_ACK));
    }
  }
  else
  { 
    // start the CAN
    FLEXCANb_MCR(flexcanBase) &= ~(FLEXCAN_MCR_HALT);
    
    // exit freeze mode and wait until it is unfrozen.
    FLEXCANb_MCR(flexcanBase) &= ~FLEXCAN_MCR_FRZ;
    while (FLEXCANb_MCR(flexcanBase) & FLEXCAN_MCR_FRZ_ACK);
  }
}


/*
 * \brief Set listen only mode on or off.
 *
 * \param mode - set listen only mode?
 *
 * \retval None.
 *
 */
void FlexCAN::setListenOnly (bool mode)
{
  
  freezeMode(true);  
  if (mode)
    FLEXCANb_CTRL1(flexcanBase) |= FLEXCAN_CTRL_LOM;
  else
    FLEXCANb_CTRL1(flexcanBase) &= ~FLEXCAN_CTRL_LOM;
  freezeMode(false);
}

 /*
  * \brief Initializes mailboxes to the requested mix of Rx and Tx boxes
  *
  * \param txboxes - How many of the 8 boxes should be used for Tx
  *
  * \retval number of tx boxes set.
  *
  */

uint32_t FlexCAN::setNumTxBoxes (uint32_t txboxes) {
    uint8_t c;
    uint32_t oldIde;

    if (txboxes > NUM_MAILBOXES - 1)
        txboxes = NUM_MAILBOXES - 1;

    if (txboxes < 1)
        txboxes = 1;

    numTxMailboxes = txboxes;

    // Inialize Rx boxen

    for (c = 0; c < NUM_MAILBOXES - numTxMailboxes; c++) {
        // preserve the existing filter ide setting

        oldIde = FLEXCANb_MBn_CS(flexcanBase, c) & FLEXCAN_MB_CS_IDE;

        FLEXCANb_MBn_CS(flexcanBase, c) = FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_RX_EMPTY) | oldIde;
    }

    // Initialize Tx boxes

    for (c = NUM_MAILBOXES - numTxMailboxes; c < NUM_MAILBOXES; c++) {
        FLEXCANb_MBn_CS(flexcanBase, c) = FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_TX_INACTIVE);
    }

    return (numTxMailboxes);
}

 /*
  * \brief Sets a per-mailbox filter. Sets both the storage and the actual mailbox.
  *
  * \param filter - a filled out filter structure
  * \param mbox - the mailbox to update
  *
  * \retval Nothing
  *
  */

void FlexCAN::setFilter (const CAN_filter_t &filter, uint8_t mbox)
{
    if (mbox < NUM_MAILBOXES - numTxMailboxes) {
        MBFilters[mbox] = filter;

        if (filter.flags.extended) {
           FLEXCANb_MBn_ID(flexcanBase, mbox) = (filter.id & FLEXCAN_MB_ID_EXT_MASK);
           FLEXCANb_MBn_CS(flexcanBase, mbox) |= FLEXCAN_MB_CS_IDE;
        } else {
           FLEXCANb_MBn_ID(flexcanBase, mbox) = FLEXCAN_MB_ID_IDSTD(filter.id);
           FLEXCANb_MBn_CS(flexcanBase, mbox) &= ~FLEXCAN_MB_CS_IDE;
        }
    }
}

 /*
  * \brief Gets a per-mailbox filter.
  *
  * \param filter - returned filter structure
  * \param mbox - mailbox selected
  *
  * \retval true if mailbox s valid, false otherwise
  *
  */

bool FlexCAN::getFilter (CAN_filter_t &filter, uint8_t mbox)
{
    if (mbox < NUM_MAILBOXES - numTxMailboxes) {
        filter.id = MBFilters[mbox].id;
        filter.flags.extended = MBFilters[mbox].flags.extended;
        filter.flags.remote = MBFilters[mbox].flags.remote;
        filter.flags.reserved = MBFilters[mbox].flags.reserved;

        return (true);
    }

    return (false);
}

/*
 * \brief Set the mailbox mask for filtering packets
 *
 * \param mask - mask to apply.
 * \param mbox - mailbox number
 *
 * \retval None.
 */

void FlexCAN::setMask (uint32_t mask, uint8_t mbox)
{
    if (mbox >= NUM_MAILBOXES - numTxMailboxes) {
        return;
    }

   /* Per mailbox masks can only be set in freeze mode so have to enter that mode if not already there. */

    if (!(FLEXCANb_MCR(flexcanBase) & FLEXCAN_MCR_FRZ_ACK)) {
        FLEXCANb_MCR(flexcanBase) |= FLEXCAN_MCR_FRZ;
        FLEXCANb_MCR(flexcanBase) |= FLEXCAN_MCR_HALT;

        while (!(FLEXCANb_MCR(flexcanBase) & FLEXCAN_MCR_FRZ_ACK))
            ;
    }

    FLEXCANb_MB_MASK(flexcanBase, mbox) = mask;

    // exit freeze mode and wait until it is unfrozen.

    FLEXCANb_MCR(flexcanBase) &= ~FLEXCAN_MCR_HALT;

    while (FLEXCANb_MCR(flexcanBase) & FLEXCAN_MCR_FRZ_ACK)
        ;
}

/*
 * \brief How many messages are available to read.
 *
 * \param None
 *
 * \retval A count of the number of messages available.
 */

uint32_t FlexCAN::available (void)
{
    return (ringBufferCount (rxRing));
}

/*
 * \brief Clear the collected statistics
 *
 * \param None
 *
 * \retval None
 */

#if defined(COLLECT_CAN_STATS)

void FlexCAN::clearStats (void)
{
    // initialize the statistics structure

    memset (&stats, 0, sizeof(stats));

    stats.enabled = false;
    stats.ringRxMax = SIZE_RX_BUFFER - 1;
    stats.ringTxMax = SIZE_TX_BUFFER - 1;
    stats.ringRxFramesLost = 0;
}
#endif

/*
 * \brief Retrieve a frame from the RX buffer
 *
 * \param msg - buffer reference to the frame structure to fill out
 *
 * \retval 0 no frames waiting to be received, 1 if a frame was returned
 */

int FlexCAN::read (CAN_message_t &msg)
{
    /* pull the next available message from the ring */

    if (removeFromRingBuffer (rxRing, msg) == true) {
        return 1;
    }

    return 0;
}

/*
 * \brief Send a frame out of this canbus port
 *
 * \param msg - the filled out frame structure to use for sending
 *
 * \note Will do one of two things - 1. Send the given frame out of the first available mailbox
 * or 2. queue the frame for sending later via interrupt. Automatically turns on TX interrupt
 * if necessary.
 *
 * Returns whether sending/queueing succeeded. Will not smash the queue if it gets full.
 */

int FlexCAN::write (const CAN_message_t &msg)
{
    uint32_t index;

    // find an available buffer

    int buffer = -1;

    for (index = NUM_MAILBOXES - numTxMailboxes - 1; index < NUM_MAILBOXES; index++) {
        if ((FLEXCANb_MBn_CS(flexcanBase, index) & FLEXCAN_MB_CS_CODE_MASK) == FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_TX_INACTIVE)) {
            buffer = index;
            break;// found one
        }
    }

    if (buffer > -1) {
        dbg_println ("Writing a frame directly.");

        writeTxRegisters (msg, buffer);
        return 1;
    } 
    // else {
    //     // no mailboxes available. Try to buffer it

    //     if (addToRingBuffer (txRing, msg) == true) {
    //         return 1;
    //     }
    // }

  // could not send the frame!

  return 0;
}

/*
 * \brief Write CAN message to the FlexCAN hardware registers.
 *
 * \param msg    - message structure to send.
 * \param buffer - mailbox number to write to.
 *
 * \retval None.
 *
 */

void FlexCAN::writeTxRegisters (const CAN_message_t &msg, uint8_t buffer)
{
    // transmit the frame

    FLEXCANb_MBn_CS(flexcanBase, buffer) = FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_TX_INACTIVE);

    if (msg.flags.extended) {
        FLEXCANb_MBn_ID(flexcanBase, buffer) = (msg.id & FLEXCAN_MB_ID_EXT_MASK);
    } else {
        FLEXCANb_MBn_ID(flexcanBase, buffer) = FLEXCAN_MB_ID_IDSTD(msg.id);
    }

    FLEXCANb_MBn_WORD0(flexcanBase, buffer) = (msg.buf[0]<<24)|(msg.buf[1]<<16)|(msg.buf[2]<<8)|msg.buf[3];
    FLEXCANb_MBn_WORD1(flexcanBase, buffer) = (msg.buf[4]<<24)|(msg.buf[5]<<16)|(msg.buf[6]<<8)|msg.buf[7];

    if (msg.flags.extended) {
        if (msg.flags.remote) {
            FLEXCANb_MBn_CS(flexcanBase, buffer) = FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_TX_ONCE) |
                                                   FLEXCAN_MB_CS_LENGTH(msg.len) | FLEXCAN_MB_CS_SRR | 
                                                   FLEXCAN_MB_CS_IDE | FLEXCAN_MB_CS_RTR;
        } else {
            FLEXCANb_MBn_CS(flexcanBase, buffer) = FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_TX_ONCE) |
                                                   FLEXCAN_MB_CS_LENGTH(msg.len) | FLEXCAN_MB_CS_SRR |
                                                   FLEXCAN_MB_CS_IDE;
        }
    } else {
        if (msg.flags.remote) {
            FLEXCANb_MBn_CS(flexcanBase, buffer) = FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_TX_ONCE) |
                                                   FLEXCAN_MB_CS_LENGTH(msg.len) | FLEXCAN_MB_CS_RTR;
        } else {
            FLEXCANb_MBn_CS(flexcanBase, buffer) = FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_TX_ONCE) |
                                                   FLEXCAN_MB_CS_LENGTH(msg.len);
        }
    }
}

/*
 * 
 * From the Data sheet:
 * If the RXERRCNT increases to a value greater than 127, it is not incremented
 * further, even if more errors are detected while being a receiver. At the next
 * successful message reception, the counter is set to a value between 119 and 127 to
 * resume to ‘Error Active’ state.
 */

uint8_t FlexCAN::readREC ()
{
  uint32_t counters = FLEXCANb_ECR(flexcanBase);
  return (counters & 0x0000FF00 ) >>8;
}

/*
 * \brief Read the transmit error counter from the FlexCAN_ECR Register
 *  
 * There are no parameters
 * 
 * \retval the Transmit Error Counter
 *
 */

uint8_t FlexCAN::readTEC ()
{
  uint32_t counters = FLEXCANb_ECR(flexcanBase);
  return (counters & 0x000000FF );
}
/*
 * \brief Read CAN message from the FlexCAN hardware registers.
 *
 * \param msg    - message structure to fill.
 * \param buffer - mailbox number to read from.
 *
 * \retval None.
 *
 */

void FlexCAN::readRxRegisters (CAN_message_t &msg, uint8_t buffer)
{
    uint32_t mb_CS = FLEXCANb_MBn_CS(flexcanBase, buffer);

    // get identifier and dlc

    msg.len = FLEXCAN_get_length (mb_CS);
    msg.flags.extended = (mb_CS & FLEXCAN_MB_CS_IDE) ? 1:0;
    msg.flags.remote = (mb_CS & FLEXCAN_MB_CS_RTR) ? 1:0;
    msg.timestamp = FLEXCAN_get_timestamp (mb_CS);
    msg.micros = micros();
    msg.rxcount++; 
    msg.flags.overrun = 0;
    msg.flags.reserved = 0;

    msg.id  = (FLEXCANb_MBn_ID(flexcanBase, buffer) & FLEXCAN_MB_ID_EXT_MASK);

    if (!msg.flags.extended) {
        msg.id >>= FLEXCAN_MB_ID_STD_BIT_NO;
    }

    // check for mailbox buffer overruns

    if (FLEXCAN_get_code (mb_CS) == FLEXCAN_MB_CODE_RX_OVERRUN) {
        msg.flags.overrun = 1;
    }

    // copy out message

    uint32_t dataIn = FLEXCANb_MBn_WORD0(flexcanBase, buffer);
    msg.buf[3] = dataIn;
    dataIn >>=8;
    msg.buf[2] = dataIn;
    dataIn >>=8;
    msg.buf[1] = dataIn;
    dataIn >>=8;
    msg.buf[0] = dataIn;

    if (msg.len > 4) {
        dataIn = FLEXCANb_MBn_WORD1(flexcanBase, buffer);
        msg.buf[7] = dataIn;
        dataIn >>=8;
        msg.buf[6] = dataIn;
        dataIn >>=8;
        msg.buf[5] = dataIn;
        dataIn >>=8;
        msg.buf[4] = dataIn;
    }
 
    for (uint32_t loop=msg.len; loop < 8; loop++ ) {
        msg.buf[loop] = 0xFF;
    }
}

/*
 * \brief Initialize the specified ring buffer.
 *
 * \param ring - ring buffer to initialize.
 * \param buffer - buffer to use for storage.
 * \param size - size of the buffer in bytes.
 *
 * \retval None.
 *
 */

void FlexCAN::initRingBuffer (ringbuffer_t &ring, volatile CAN_message_t *buffer, uint32_t size)
{
    ring.buffer = buffer;
    ring.size = size;
    ring.head = 0;
    ring.tail = 0;
}

/*
 * \brief Add a CAN message to the specified ring buffer.
 *
 * \param ring - ring buffer to use.
 * \param msg - message structure to add.
 *
 * \retval true if added, false if the ring is full.
 *
 */

bool FlexCAN::addToRingBuffer (ringbuffer_t &ring, const CAN_message_t &msg)
{
    uint16_t nextEntry;

    nextEntry = (ring.head + 1) % ring.size;

    /* check if the ring buffer is full */

    if (nextEntry == ring.tail) {
        return (false);
    }

    /* add the element to the ring */

    memcpy ((void *)&ring.buffer[ring.head], (void *)&msg, sizeof (CAN_message_t));

    /* bump the head to point to the next free entry */

    ring.head = nextEntry;

    return (true);
}

/*
 * \brief Remove a CAN message from the specified ring buffer.
 *
 * \param ring - ring buffer to use.
 * \param msg - message structure to fill in.
 *
 * \retval true if a message was removed, false if the ring is empty.
 *
 */

bool FlexCAN::removeFromRingBuffer (ringbuffer_t &ring, CAN_message_t &msg)
{

    /* check if the ring buffer has data available */

    if (isRingBufferEmpty (ring) == true) {
        return (false);
    }

    /* copy the message */

    memcpy ((void *)&msg, (void *)&ring.buffer[ring.tail], sizeof (CAN_message_t));

    /* bump the tail pointer */

    ring.tail = (ring.tail + 1) % ring.size;

    return (true);
}

/*
 * \brief Check if the specified ring buffer is empty.
 *
 * \param ring - ring buffer to use.
 *
 * \retval true if the ring contains data, false if the ring is empty.
 *
 */

bool FlexCAN::isRingBufferEmpty (ringbuffer_t &ring)
{
    if (ring.head == ring.tail) {
        return (true);
    }

    return (false);
}

/*
 * \brief Count the number of entries in the specified ring buffer.
 *
 * \param ring - ring buffer to use.
 *
 * \retval a count of the number of elements in the ring buffer.
 *
 */

uint32_t FlexCAN::ringBufferCount (ringbuffer_t &ring)
{
    int32_t entries;

    entries = ring.head - ring.tail;

    if (entries < 0) {
        entries += ring.size;
    }

    return ((uint32_t)entries);
}


/*
 * \brief Interrupt service routine for the FlexCAN class message events.
 *
 * \param None.
 *
 * \retval None.
 *
 */
void FlexCAN::message_isr (void)
{
    uint32_t status = FLEXCANb_IFLAG1(flexcanBase);
    uint8_t controller = 0;
    uint32_t i;
    CAN_message_t msg;
    bool handledFrame;
    CANListener *thisListener;
#if defined(COLLECT_CAN_STATS)
    uint32_t rxEntries;
#endif

    // determine which controller we're servicing

#if defined (INCLUDE_FLEXCAN_CAN1)
    if (flexcanBase == FLEXCAN1_BASE)
        controller = 1;
#endif

    // a message either came in or was freshly sent. Figure out which and act accordingly.

    for (i = 0; i < NUM_MAILBOXES; i++) {

        // skip mailboxes that haven't triggered an interrupt

        if ((status & (1 << i)) == 0) {
            continue;
        }

        // examine the reason the mailbox interrupted us

        uint32_t code = FLEXCAN_get_code (FLEXCANb_MBn_CS(flexcanBase, i));

        switch (code) {

        case FLEXCAN_MB_CODE_RX_FULL:    // rx full, Copy the frame to RX buffer
        case FLEXCAN_MB_CODE_RX_OVERRUN: // rx overrun. Incomming frame overwrote existing frame.
            readRxRegisters (msg, i);
            
            
            handledFrame = false;

            // track message use count if collecting statistics

#if defined(COLLECT_CAN_STATS)
            if (stats.enabled == true) {
                stats.mb[i].refCount++;

                if (msg.flags.overrun) {
                    stats.mb[i].overrunCount++;
                }
            }
#endif

            // First, try and handle via callback. If callback fails then buffer the frame.

            for (uint32_t listenerPos = 0; listenerPos < SIZE_LISTENERS; listenerPos++) {
                thisListener = listener[listenerPos];

                // process active listeners

                if (thisListener != NULL) {

                    // call the handler if it's active for this mailbox

                    if (thisListener->callbacksActive & (1 << i)) {
                        handledFrame |= thisListener->frameHandler (msg, i, controller);
                    } else if (thisListener->callbacksActive & (1 << 31)) {
                        handledFrame |= thisListener->frameHandler (msg, -1, controller);
                    }
                }
            }

            // if no objects caught this frame then queue it in the ring buffer

            if (handledFrame == false) {
                if (addToRingBuffer (rxRing, msg) != true) {
                    // ring buffer is full, track it

                    dbg_println ("Receiver buffer overrun!");

                    #if defined(COLLECT_CAN_STATS)
                    if (stats.enabled == true) {
                        stats.ringRxFramesLost++;
                    }
                    #endif
                }
            }

            #if defined(COLLECT_CAN_STATS)
            if (stats.enabled == true) {

                // track the high water mark for the receive ring buffer

                rxEntries = ringBufferCount (rxRing);

                if (stats.ringRxHighWater < rxEntries) {
                    stats.ringRxHighWater = rxEntries;
                }
            }
            #endif

            // it seems filtering works by matching against the ID stored in the mailbox
            // so after a frame comes in we've got to refresh the ID field to be the filter ID and not the ID
            // that just came in.

            if (MBFilters[i].flags.extended) {
                FLEXCANb_MBn_ID(flexcanBase, i) = (MBFilters[i].id & FLEXCAN_MB_ID_EXT_MASK);
            } else {
                FLEXCANb_MBn_ID(flexcanBase, i) = FLEXCAN_MB_ID_IDSTD(MBFilters[i].id);
            }
            break;

        case FLEXCAN_MB_CODE_TX_INACTIVE: // TX inactive. Just chillin' waiting for a message to send. Let's see if we've got one.

            // if there is a frame in the queue then send it
            if (isRingBufferEmpty (txRing) == false) {
                if (removeFromRingBuffer (txRing, msg) == true) {
                    writeTxRegisters (msg, i);
                }
            }
            break;

        // currently unhandled events

        case FLEXCAN_MB_CODE_RX_INACTIVE:       // inactive Receive box. Must be a false alarm!?
        case FLEXCAN_MB_CODE_RX_BUSY:           // mailbox is busy. Don't touch it.
        case FLEXCAN_MB_CODE_RX_EMPTY:          // rx empty already. Why did it interrupt then?
        case FLEXCAN_MB_CODE_TX_ABORT:          // TX being aborted.
        case FLEXCAN_MB_CODE_TX_RESPONSE:       // remote request response (deprecated)
        case FLEXCAN_MB_CODE_TX_ONCE:           // TX mailbox is full and will be sent as soon as possible
        case FLEXCAN_MB_CODE_TX_RESPONSE_TEMPO: // remote request junk again. Go away.
            break;

        default:
            break;
        }
    }

    // writing the flag value back to itself clears all flags

    FLEXCANb_IFLAG1(flexcanBase) = status;
}

/*
 * \brief Attach an object to the listening list.
 *
 * \param listener - pointer to listening object
 *
 * \retval true if listener added to list, false if the list is full.
 *
 */

bool FlexCAN::attachObj (CANListener *listener)
{
    uint32_t i;

    for (i = 0; i < SIZE_LISTENERS; i++) {
        if (this->listener[i] == NULL) {
            this->listener[i] = listener;
            listener->callbacksActive = 0;

            return true;
        }
    }

    return false;
}

/*
 * \brief Detatch an object from the listening list.
 *
 * \param listener - pointer to listening object
 *
 * \retval true if listener removed from list, false if object not found.
 *
 */

bool FlexCAN::detachObj (CANListener *listener)
{
    uint32_t i;

    for (i = 0; i < SIZE_LISTENERS; i++) {
        if (this->listener[i] == listener) {
            this->listener[i] = NULL;

            return true;
        }
    }

    return false;
}

void FlexCAN::bus_off_isr (void)
{
  uint32_t status = FLEXCANb_ESR1 (flexcanBase);

  // Clear the ESR Interrupt flag  
  FLEXCANb_ESR1(flexcanBase) = status; 
}

/*
 * \brief Interrupt service routine for FlexCAN class device errors.
 *  ID Composed of error flags from Linux Socket CAN
 *
 * \param None.
 *
 * \retval None.
 *
 */

void FlexCAN::error_isr (void)
{ 
  uint32_t status = FLEXCANb_ESR1 (flexcanBase);
  if (report_errors){
    CAN_message_t msg;
    msg.id = CAN_ERR_FLAG; //Set this to show this is an error id
    msg.len = 8;
    msg.ext = 1;
    memset(&msg.buf, 0, 8);

    // an acknowledge error happened - frame was not ACK'd
    if (status & FLEXCAN_ESR_ACK_ERR) {
      msg.id |= CAN_ERR_ACK; /* received no ACK on transmission */
    }

    // A bit stuffing error was detected.
    if (status & FLEXCAN_ESR_STF_ERR) {
      msg.id |= CAN_ERR_PROT; /* protocol violations / data[2..3] */
      msg.buf[2] |= CAN_ERR_PROT_STUFF;  /* bit stuffing error */
    }

    // Form Error
    // This bit indicates that a Form Error has been detected by the receiver node, that is, a fixed-form bit field
    // contains at least one illegal bit.
    if (status & FLEXCAN_ESR_FRM_ERR) {
      msg.id |= CAN_ERR_PROT; /* protocol violations / data[2..3] */
      msg.buf[2] |= CAN_ERR_PROT_FORM; /* frame format error */
    }

    // Cyclic Redundancy Check Error
    // This bit indicates that a CRC Error has been detected by the receiver node, that is, the calculated CRC is
    // different from the received.
    if (status & FLEXCAN_ESR_CRC_ERR) {
      msg.id |= CAN_ERR_PROT; /* protocol violations / data[2..3] */
      msg.buf[3] |= CAN_ERR_PROT_LOC_CRC_SEQ; /* CRC sequence */
      msg.buf[2] |= CAN_ERR_PROT_BIT;/* single bit error - Not actually part of FlexCAN*/ 
    }

    // Bit0 Error
    // This bit indicates when an inconsistency occurs between the transmitted and the received bit in a
    // message.
    if (status & FLEXCAN_ESR_BIT0_ERR) {
      msg.id |= CAN_ERR_PROT; /* protocol violations / data[2..3] */
      msg.buf[2] |= CAN_ERR_PROT_BIT0; /* unable to send dominant bit */ 
    }

    // Bit1 Error
    // TThis bit indicates when an inconsistency occurs between the transmitted and the received bit in a
    // message.
    // NOTE: This bit is not set by a transmitter in case of arbitration field or ACK slot, or in case of a node
    // sending a passive error flag that detects dominant bits.
    if (status & FLEXCAN_ESR_BIT1_ERR) {
      msg.id |= CAN_ERR_PROT; /* protocol violations / data[2..3] */
      msg.buf[2] |= CAN_ERR_PROT_BIT1; /* unable to send dominant bit */
    }
    addToRingBuffer (rxRing, msg);
  }  
  // Clear the ESR Interrupt flag  
  FLEXCANb_ESR1(flexcanBase) = status; 
}

/*
 * \brief Interrupt service routine for the FlexCAN class transmit warnings.
 *
 * \param None.
 *
 * \retval None.
 *
 */

void FlexCAN::tx_warn_isr (void)
{
  uint32_t status = FLEXCANb_ESR1 (flexcanBase);
  // CAN_ERR_CRTL 0x00000004U /* controller problems / data[1]    */
  /* error status of CAN-controller / data[1] */
  //  CAN_ERR_CRTL_TX_WARNING 0x08 /* reached warning level for TX errors */
  FLEXCANb_ESR1(flexcanBase) = status; 
}

/*
 * \brief Interrupt service routine for the FlexCAN class receive warnings.
 *
 * \param None.
 *
 * \retval None.
 *
 */

void FlexCAN::rx_warn_isr (void)
{

  uint32_t status = FLEXCANb_ESR1 (flexcanBase);

  // If many rx errors are occurring, it is likely the baud rate is off
  Serial.print("RX Warn Interrupt. Setting new baudrate to ");
  Serial.println(get_baud_rate());
  //CAN_ERR_CRTL 0x00000004U /* controller problems / data[1]    */
  /* error status of CAN-controller / data[1] */
  // CAN_ERR_CRTL_RX_WARNING 0x04 /* reached warning level for RX errors */
  FLEXCANb_ESR1(flexcanBase) = status; 
}

/*
 * \brief Interrupt service routine for the FlexCAN class device wakeup.
 *
 * \param None.
 *
 * \retval None.
 *
 */

void FlexCAN::wakeup_isr (void)
{ 
  uint32_t status = FLEXCANb_ESR1 (flexcanBase);

  FLEXCANb_ESR1(flexcanBase) = status;   
}

/*
 * \brief Interrupt handler for FlexCAN can0 message events.
 *
 * \param None.
 *
 * \retval None.
 *
 */

void can0_message_isr (void)
{
    Can0.message_isr ();
}

/*
 * \brief Interrupt handler for FlexCAN can0 bus off event.
 *
 * \param None.
 *
 * \retval None.
 *
 */

void can0_bus_off_isr (void)
{
    Can0.bus_off_isr ();
}

/*
 * \brief Interrupt handler for FlexCAN can0 error events.
 *
 * \param None.
 *
 * \retval None.
 *
 */

void can0_error_isr (void)
{
    Can0.error_isr ();
}

/*
 * \brief Interrupt handler for FlexCAN can0 transmit warning event.
 *
 * \param None.
 *
 * \retval None.
 *
 */

void can0_tx_warn_isr (void)
{
    Can0.tx_warn_isr ();
}

/*
 * \brief Interrupt handler for FlexCAN can0 receive warning event.
 *
 * \param None.
 *
 * \retval None.
 *
 */

void can0_rx_warn_isr (void)
{
    Can0.rx_warn_isr ();
}

/*
 * \brief Interrupt handler for FlexCAN can0 device wakeup event.
 *
 * \param None.
 *
 * \retval None.
 *
 */

void can0_wakeup_isr (void)
{
    Can0.wakeup_isr ();
}

#if defined(INCLUDE_FLEXCAN_CAN1)

/*
 * \brief Interrupt handler for FlexCAN can1 message events.
 *
 * \param None.
 *
 * \retval None.
 *
 */

void can1_message_isr (void)
{
    Can1.message_isr ();
}

/*
 * \brief Interrupt handler for FlexCAN can1 bus off event.
 *
 * \param None.
 *
 * \retval None.
 *
 */

void can1_bus_off_isr (void)
{
    Can1.bus_off_isr ();
}

/*
 * \brief Interrupt handler for FlexCAN can1 error events.
 *
 * \param None.
 *
 * \retval None.
 *
 */

void can1_error_isr (void)
{
    Can1.error_isr ();
}

/*
 * \brief Interrupt handler for FlexCAN can1 transmit warning event.
 *
 * \param None.
 *
 * \retval None.
 *
 */

void can1_tx_warn_isr (void)
{
    Can1.tx_warn_isr ();
}

/*
 * \brief Interrupt handler for FlexCAN can1 receive warning event.
 *
 * \param None.
 *
 * \retval None.
 *
 */

void can1_rx_warn_isr (void)
{
    Can1.rx_warn_isr ();
}

/*
 * \brief Interrupt handler for FlexCAN can1 device wakeup event.
 *
 * \param None.
 *
 * \retval None.
 *
 */

void can1_wakeup_isr (void)
{
    Can1.wakeup_isr ();
}

#endif /* INCLUDE_FLEXCAN_CAN1 */

/*
 * \brief CANListener constructor
 *
 * \param None.
 *
 * \retval None.
 *
 */

CANListener::CANListener ()
{
    // none. Bitfield were bits 0-15 are the mailboxes and bit 31 is the general callback

    callbacksActive = 0;
}

/*
 * \brief Default CAN received frame handler.
 *
 * \param frame - CAN frame to process.
 * \param mailbox - mailbox number frame arrived at.
 * \param controller - controller number frame arrived from.
 *
 * \retval true if frame was handled, false otherwise.
 *
 */

bool CANListener::frameHandler (CAN_message_t &frame, int mailbox, uint8_t controller)
{

    /* default implementation that doesn't handle frames */

    return (false);
}

/*
 * \brief Indicate mailbox has an active callback.
 *
 * \param mailBox - mailbox number.
 *
 * \retval None.
 *
 */

void CANListener::attachMBHandler (uint8_t mailBox)
{
    if ((mailBox >= 0) && (mailBox < NUM_MAILBOXES)) {
        callbacksActive |= (1L << mailBox);
    }
}

/*
 * \brief Clear callback indicator for a mailbox.
 *
 * \param mailBox - mailbox number.
 *
 * \retval None.
 *
 */

void CANListener::detachMBHandler (uint8_t mailBox)
{
    if ((mailBox >= 0) && (mailBox < NUM_MAILBOXES)) {
        callbacksActive &= ~(1L << mailBox);
    }
}

/*
 * \brief Set general purpose callback indicator.
 *
 * \param None.
 *
 * \retval None.
 *
 */

void CANListener::attachGeneralHandler (void)
{
    callbacksActive |= (1L << 31);
}

/*
 * \brief Clear general purpose callback indicator.
 *
 * \param None.
 *
 * \retval None.
 *
 */

void CANListener::detachGeneralHandler (void)
{
    callbacksActive &= ~(1L << 31);
}

import subprocess

while True:
    p = subprocess.Popen(["top", "-b", "-n", "1", "-c"], stdout=subprocess.PIPE)
    line = p.stdout.readline().decode('utf-8')
    i=0
    cpuTotal=0.0
    while line:
        if i>6: #i=7 is the first process
            #print(line)
            cpuPerc = [x for x in line.split(" ") if x][8] #removes empty strings, index 8 is cpuPerc
            #print(cpuPerc)
            cpuTotal+= float(cpuPerc)
            #if cpuTotal > 100: print(line)
        i+=1
        line = p.stdout.readline().decode('utf-8')
    print("Total CPU Utilization: {}%".format(cpuTotal))
    #if cpuTotal > 100: break;
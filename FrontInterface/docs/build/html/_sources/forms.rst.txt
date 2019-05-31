Forms
=========

.. py:currentmodule:: experimenteditor.forms


Form Classes
______________


.. py:class:: ExperimentForm(ModelForm)
		
	Creates an :py:class:`Experiment` instance from a user title.  
		
.. py:class:: DuplicateExperimentForm(ModelForm)

	Duplicates an existing :py:class:`Experiment` .  TODO: fix :py:attr:`dup_experiment` 's ``queryset`` so it doesn't return **all** experiments, including ones that aren't the user's.
	
	.. py:attribute:: dup_experiment
		
		The :py:class:`Experiment` to be duplicated.
		
	.. py:attribute:: new_title
	
		The new title of the experiment.  If left blank, this will be the old experiment's title plus "Copy".  
		
.. py:class:: SSSCommandForm(ModelForm)

		Creates/Updates an :py:class:`OneTimeSSSCommand` instance.  TODOS:  
		
			1. remove looping functionality from model and form, was just confusing...
		
			2. allow user to edit an EndExperiment command's delay without deleting and re-creating the command
	
	.. py:attribute:: quantity
	
		Analagous to :py:class:`experimenteditor.models.OneTimeSSSCommand` 's ``quantity`` attribute, javascript ensures that this field is blank if the command takes no arguments (Start/Stop Ignition, etc.)
		
		If there is more than one argument to a command (braking is the only one at this point), each value must be comma-separated.
		
		Every value entered must be castable to ``float``.
	
	.. py:attribute:: parent_experiment
		
		The :py:class:`experimenteditor.models.Experiment` this command belongs to.  Auto-filled.
		
	``is_repeated``, ``repeat_count`` and ``repeat_delay`` are functional but deprecated.
	
.. py:class:: CANCommandForm(ModelForm)

	Creates/Updates a :py:class:`experimenteditor.models.CANCommand` instance.  TODOS:
	
		1. Un-split the :py:attr:`message_id` field into one ``RegexField`` (should be easy, deprecates the Four-byte widget and field).
	
	For all fields see :py:class:`experimenteditor.models.CANCommand` for details on what they are.
	
.. py:class:: CANGenCommandForm(ModelForm)
	
	Creates/Updates a :py:class:`experimenteditor.models.CANGenCommand` instance.  TODOS:
		
		1. Get the model help text to display (somehow) on the form page, immensely helpful.  Probably too long to fit inside the box, currently.
		
	For all class attribute explanations see :py:class`experimenteditor.models.CANGenCommand` for more details. 
	
.. py:class:: ExperimentEventForm(ModelForm)

	This form class does a few things:
	
		1. Creates a :py:class:`schedule.models.Event` instance that describes when the experiment starts and for how long it lasts.  Timezone conversions are handled (to a point, see source for details).  

		2. Calls/schedules :py:func:`experimenteditor.tasks.send_experiment_json` and :py:func:`experimenteditor.tasks.send_reminder_email` if the user decides to send a reminder email to themself.
	
		3. Creates a :py:class:`experimenteditor.models.RunResult` object that will contain the log/any data the user decides to plot through the "Visualize" page.  This is tied automatically to the :py:class:`experimenteditor.models.Experiment` instance upon creation.
		
		4. Creates an :py:class:`experimenteditor.models.ExperimentSchedulingInfo` instance that ties the newly created :py:class:`schedule.models.Event` to the :py:class:`experimenteditor.models.Experiment` and marks it as scheduled until the experiment is successfully run.
		
		5. Tries to create empty :py:class:`experimenteditor.models.ObservableQuantity` objects that correspond to what the user graphed last run by getting the most recent :py:class:`experimenteditor.models.RunResult` and copying all its associated :py:class:`experimenteditor.models.ObservableQuantity` objects from the last run to the current run.  

	TODOS:
	
		1. Fix item #5, currently not functional, but would be very nice if one could just re-schedule and have it save your graph settings somehow so you only have to hit "Visualize" once unless you want to make changes. 
		
.. py:class:: GraphableQuantityChoiceForm(ModelForm)

	This form class is the "Visualize" page.  It sets up the :py:class:`experimenteditor.models.ObservableQuantity` objects so that they can receive plot data.

	Currently tied to an SPN (and therefore, a :py:class:`experimenteditor.models.SPNPGNEntry`) internally so that titles are auto-generated based on SPN and the spreadsheet imported into the database. 

	
Widgets/Fields
____________________

There are two widget/field pairs, but they work very similarly.  They allow multiple text fields (hex bytes) to be compressed into one hex string on the Python side of things.  They also allow a single hex string to be chopped up into byte-sized pieces, which fill each individual text field on the HTML side of things.  I will describe what the eight-byte variant does, and the four-byte variant is almost identical.

.. py:class:: MultiHexEightByteField(django.forms.MultiValueField)

	.. py:function:: compress(data_list)
	
		Compresses the data from a list of byte strings into one 16-character hex string.
		
		:param list data_list: A list of 2-character strings.  Validation for this is done in :py:class:`CANCommandForm` through Django's ``RegexField``.

.. py:class:: MultiHexEightByteWidget(django.forms.MultiWidget)

	.. py:function:: __init__(attrs=None, mode=0)
	
		Initializes the widget, applying any attributes if needed. See the django documentation for ``django.forms.MultiWidget`` for more details on the parameters.
		
	.. py:function:: decompress(value)
	
		Takes a value (a hex string in this case) and splits it up into a list of 2-character chunks.  Program defensively, because :py:func:`decompress` is often called while ``value`` is ``None``.
  		
		

Models
==========


.. py:currentmodule:: experimenteditor.models

.. py:class:: Experiment
	
	.. py:attribute:: exp_pk  

	The primary key of the experiment.  Using ``pk`` or ``id`` in any experiment query **will** fail.  Use ``exp_pk`` instead.

	.. py:attribute:: experiment_title  

	The title of the experiment.

	.. py:attribute:: experiment_created_date  

	A Python ``datetime`` that is set upon filling out a title and hitting the "Create" button.  Also applies to duplicated experiments.

	.. py:attribute:: created_by  

	A reference to a :py:class:`django.contrib.auth.models.User` instance that indicates who created an experiment.  This determines what users can see on the "My Experiments" page.

	.. py:attribute:: is_scheduled 

	A boolean that is true from the time a user schedules an experiment to the time it ends and is done processing.  Is used to disable some functions of the "My Experiments" page.

	.. py:function:: Experiment.has_results()

	:return: Whether or not the experiment has associated logs/plots.  Controls whether or not the "View Results" page is available from the "My Experiments" page.
	
	:rtype: bool

	.. py:function:: Experiment.get_results()

	:return: A QuerySet of :py:class:`ObservableQuantity` objects that are currently associated with the experiment.
	
	:rtype: QuerySet

	.. py:function:: Experiment.get_results_url()

	:return: An absolute url to the experiment's "View Results" page.

	:rtype: string

	.. py:function:: Experiment.slugify_name()

	:return: A CamelCase version of the title concatenated with the primary key of the experiment.  Used in :py:func:`experimenteditor.tasks.send_experiment_json()`.

	:rtype: string

	.. py:function:: Experiment.has_end()

	:return: True if the experiment can be scheduled due to it ending, False if it can't.

	:rtype: bool

	.. py:function:: Experiment.get_all_commands_list()

	:return: A list of every command that is associated with the experiment.  

	:rtype: list of :py:class:`Command` objects

.. py:class:: Command
 
	.. py:attribute:: parent_experiment

	References the experiment that the command is in.

	.. py:attribute:: delay

	A Python ``Decimal`` object that supports numbers up to 99999.999999.  In practice, this will be 14400 (4 hours in seconds) or below (but not negative).  This sets when the command goes off, in seconds, relative to the start of the experiment.

.. py:class:: OneTimeSSSCommand(experimenteditor.models.Command)

	.. py:attribute:: command_choice

	A :py:class:`django.db.models.ChoiceField` that internally stores what command they would like to execute and externally displays that command and what units the arguments are in (see :py:attr:`quantity`)  

	.. py:attribute:: quantity
		
	A :py:class:`experimenteditor.models.SeperatedValuesField` that takes a comma-seperated list of values from the frontend. Normalizes to a Python list of strings. Is validated for number of arguments in :file:`experimenteditor/forms.py` on command creation/edit.

	.. py:attribute:: is_repeated

	Deprecated, along with :py:attr:`repeat_delay` and :py:attr:`repeat_count` due to looping issues.

.. py:class:: CANCommand(experimenteditor.models.Command)

	.. py:attribute:: length

	A :py:class:`ChoiceField` that specifies the message length (from 0-8 bytes).

	.. py:attribute:: message

	The hexidecimal message string to be sent to the specified ID.  Due to the javascript in the CANCommand form, cannot be longer than 2 times the length (in characters).

	.. py:attribute:: message_id

	The hexidecimal id string. Is only 8 characters long, no more, no less.

	.. py:attribute:: interface

	The interface (currently only can1 since we don't know whether or not can0 will be useful) the message is sent to.  Stored as an integer (1, in this case).

	.. py:attribute:: is_extended_can

	A boolean that determines if the message is extended CAN or not.  The setting of the first bit is taken care of by the :py:func:`experimenteditor.tasks.send_experiment_json()` task at send time.

.. py:class:: CANGenCommand(experimenteditor.models.Command)
	
	.. py:attribute:: interface

	See :py:attr:`CANCommand.interface`.

	.. py:attribute:: gap

	The gap (in ms) between consecutive generated messages.  200 ms if left blank, can be 0 for ASAP message explosions.

	.. py:attribute:: generate_extended_can

	See :py:attr:`CANCommand.is_extended_can`.

	.. py:attribute:: send_rtr_frame

	Boolean.  If you figure out what this is, add it to the docs.

	.. py:attribute:: message_length

	0-8, blank for random, or 'i' for increasing.  

	.. py:attribute:: message_id

	See :py:attr:`CANCommand.message_id`, except can be left blank or set to "i" for increasing.

	.. py:attribute:: can_data
	
	0-8 hex bytes, "i" for increasing, or leave blank for random data.  

	.. py:attribute:: number_of_can_frames_before_end

	Specifies the number of CAN frames (sent? sent+received? just received?) before this CanGEN command terminates.

.. py:class:: ObservableQuantity

	.. py:attribute:: related_experiment

	Contains a reference to the :py:class:`Experiment` this plot is associated with.

	.. py:attribute:: related_run

	Contains a reference to the :py:class:`RunResult` this plot is associated with.

	.. py:attribute:: xdata

	A :py:class:`SeperatedValuesField` that contains all the x coordinates/timestamps.  Normalizes to a Python list of strings, so be sure to cast each element to a float before doing anything numeric.

	.. py:attribute:: ydata

	A :py:class:`SeperatedValuesField` that contains all the y coordinates of the scatter plot.  Right now can only visualize axle-based wheel speed in mph, so is guaranteed to be that.  Also normalizes to a list of strings.

	.. py:attribute:: entry

	A reference to an :py:class:`SPNPGNEntry` that specifies what SPN/PGN the plot is representing.

	.. py:attribute:: related_run

	Specifies what :py:class:`RunResult` object this observable quantity is bound to.  Is ``None`` on first creation through the "Visualize" page, but is assigned later on experiment schedule.

	.. py:function:: ObservableQuantity.gen_plot_html()

	Generates a plotly html div as a string to be used in a template.  Use the Django ``safe`` filter when displaying results of this method in templates.  

	:return: A string that contains div HTML to render a plot using plotly.js.

.. py:class:: RunResult

	.. py:attribute:: experiment

	Contains a reference to the :py:class:`Experiment` this plot/log set is associated with.

	.. py:attribute:: event

	Contains a reference to the associated :py:class:`schedule.models.Event`.  This is used to get when the :py:class:`RunResult` was created.

	.. py:attribute:: log

	Contains the log for this experiment run, as a string.









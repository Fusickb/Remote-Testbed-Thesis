.. py:currentmodule:: experimenteditor.tasks

Tasks
==========

Tasks are basically functions that can be called whenever and executed later, through the :py:attr:`schedule` parameter of the task decorator.  You can pass in either a python ``datetime`` or a python ``timedelta`` through :py:attr:`schedule` for absolute or relative timing.  For more on the inner workings of this system, see `this documentation page <http://django-background-tasks.readthedocs.io/en/latest/>`    

Global Variables
~~~~~~~~~~~~~~~~~~~~

.. py:data:: remindermessage

A format string that constructs the email body of an email reminder message.  It takes two arguments, :py:attr:`start_humanized` and :py:attr:`end_humanized`, and those should contain the humanized period of time from now to the start and end of the event through :py:func:`arrow.humanize` .

.. py:data:: expfinishedmessage

A format string that constructs the (now-invisible) email body of the "this experiment has finished" message.  Takes two arguments: the title of the experiment through :py:attr:`title` and the link to the results page through :py:attr:`link` . 

URLs
~~~~~~

.. py:data:: ENGINEBEAGLEBONEIP

This contains the URL that all experiments are POSTed to.  No :py:func:`str.format` arguments here.

.. py:data:: LOGGERIP

Not so aptly named, this contains the url that gets the plot data for axle-based vehicle speed.  Feel free to rename.  Takes one :py:func:`str.format` argument, :py:attr:`expname`, which should be the result of :py:func:`experimenteditor.models.Experiment.slugify_name` .

.. py:data:: FULLLOGIP

This contains the log ip.  Takes two :py:func:`str.format` arguments, :py:attr:`expname` as described in :py:data:`LOGGERIP`, and :py:attr:`chunkidx` .           


Task Functions
~~~~~~~~~~~~~~~~~~~~~

.. py:function:: send_experiment_reminder(experimentid)
	
	Send a reminder email to the user associated with an experiment (if they decide to send one).

	:param int experimentid: The exp_pk of the experiment model that belongs to the user to be reminded.

.. py:function:: send_experiment_json(experimentid)

	HTTP POST an experiment to beaglebone 22.  

	:param int experimentid: The :py:attr:`Experiment.exp_pk` of the experiment to be POSTed.

	Note: if you want the error messages to be more user-side and verbose, use :py:func:`notify.send` like in the other tasks with :py:attr:`level` set to 'error'.

.. py:function:: make_experiment_available(experimentid, runid)

	HTTP GET an experiment and any associated data the user requests through the "Visualize" page.

	:param int experimentid: The exp_pk of the experiment that has been completed.

	:param int runid: The primary ky of the RunResult object that the log/plot data will be stored in.

.. py:function:: send_complete_mail(experimentid, resultsurl)

	Send an email upon experiment competion to remind the user their experiment is done.  Currently sends the subject fine, but provides no body for some reason.  
	
	:param int experimentid: The exp_pk of the experiment that just completed.
	
	:param str resultsurl:  The full absolute url, including domain name/IP address and protocol (http or https) that directs to a page where the user can view their results.
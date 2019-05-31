(Re-)Installation and Quick Start
======================================

After cloning the frontend repository, you're going to want to run ``python3 -m pip install -r requirements.txt`` after navigating to the folder you cloned the repo to.  The only package that's modified is django-scheduler, and that is to support 12-hour time and associate events with experiments.  To make these modifications, you're going to want to do the following:

1. Install django-scheduler as normal through pip.  You do not have to do all the bower stuff, as jQuery and Bootstrap are in the project already as static files.
2. Find where django-scheduler installed to.  For me, it installed in the :file:`/usr/local/lib/python3.6/site-packages/schedule/` directory, but for other computers/OSes it may be different.
3. Replace its contents with :download:`this <django-scheduler-modified.zip>`.  Make sure the directory structure is the same before and after.

Now you're safe to run the website.  To do so, simply navigate to :file:`path/to/repo/trucksite/` and run ``python3 manage.py runserver`` with any additional arguments.  In a seperate terminal, run ``python3 manage.py process_tasks`` to start the background-tasks scheduler.  From there, you should have a fully functional frontend.    
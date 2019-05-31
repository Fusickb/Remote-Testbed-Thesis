from django.db import models
from django.contrib.auth.models import User
from django.utils import timezone
from django.urls import reverse
from django.core.exceptions import ObjectDoesNotExist, ValidationError
from datetime import timedelta
from django.db.models.query_utils import DeferredAttribute
import arrow
import re
from itertools import chain
from schedule.models import Event
from plotly.offline import plot
from django.urls import reverse_lazy
from django.utils.translation import ugettext_lazy as _
import codecs
import os

LOG_DIRECTORY = 'testbedLogs'

EMAIL_REMINDER_CHOICES = (
		('', 'No Reminder'),
		('1day', '1 Day Before'),
		('1hr', '1 Hour Before'),
		('30min', '30 Minutes Before'),
		('15min', '15 Minutes Before'),
		('10min', '10 Minutes Before')
		)
class SeparatedValuesField(models.TextField):
	def __init__(self, *args, **kwargs):
		self.token = kwargs.pop('token', ',')
		super(SeparatedValuesField, self).__init__(*args, **kwargs)

	def to_python(self, value):
		if not value: return
		if isinstance(value, list):
			return value
		return value.split(self.token)

	def get_db_prep_value(self, value, connection, prepared=False):
		if not value: return
		assert(isinstance(value, list) or isinstance(value, tuple))
		return self.token.join([str(s) for s in value])

	def value_to_string(self, obj):
		value = self._get_val_from_obj(obj)
		return self.get_db_prep_value(value)

CHART_LINE_COLORS = ['hsl(164.7, 100%, 36.9%)', 'hsl(0, 100%, 36.9%)', 'hsl(253, 100%, 36.9%)', 'hsl(31, 100%, 36.9%)' 'hsl(281, 100%, 36.9%)', 'hsl(76, 100%, 36.9%)']
# Create your models here.
class Experiment(models.Model):
	exp_pk = models.AutoField(primary_key=True)
	experiment_title = models.CharField(max_length=256, verbose_name='Title')
	experiment_created_date = models.DateTimeField(auto_now_add=True)
	created_by = models.ForeignKey(User, on_delete=models.CASCADE)
	is_scheduled = models.BooleanField(default=False)

	#define other fields here
	def __str__(self):
		return self.experiment_title
	def get_absolute_url(self):
		return reverse('edit_experiment', kwargs={'exp_pk': self.exp_pk})
	def delete(self):
		#print("Deleting")
		#print(os.getcwd())
		for root, dirs, files in os.walk('./'+LOG_DIRECTORY):
			for filename in files: #for every file in log directory
				#print("Name:",self.slugify_name())
				#print(filename+'\n')
				if self.slugify_name() in filename: #if the experiment name is found within a file
					print("Removing file:",filename)
					os.remove(LOG_DIRECTORY+"/"+filename) #delete the associated log file
		#print("Deleting period over")
	
		if hasattr(self, 'scheduling_info_for'):
			self.scheduling_info_for.related_event.delete()
			self.scheduling_info_for.delete()
		super(Experiment, self).delete()
	
	def has_results(self):
		return RunResult.objects.filter(experiment=self).exists()

	def get_results(self):
		Experiment.refresh_from_db(self)
		return ObservableQuantity.objects.filter(related_experiment=self)

	def get_results_url(self):
		return reverse_lazy('display_results', args=[self.exp_pk])

	def slugify_name(self):
		strlist = self.experiment_title.split(' ')
		newstrlist = []
		regex = re.compile(r'[^0-9a-zA-Z]+')
		for i in range(len(strlist)):
			newstrlist.append(re.sub(regex, '', strlist[i].capitalize()))
		newstrlist.append(str(self.exp_pk))
		return ''.join(newstrlist)

	def has_end(self):
		return OneTimeSSSCommand.objects.filter(parent_experiment=self, commandchoice='EndExperiment(0)').exists()

	def get_all_commands_list(self):
		return list(chain(OneTimeSSSCommand.objects.filter(parent_experiment=self), CANCommand.objects.filter(parent_experiment=self), CANGenCommand.objects.filter(parent_experiment=self)))

class ExperimentSchedulingInfo(models.Model):
	related_event = models.OneToOneField(Event, on_delete=models.CASCADE, related_name='scheduling_event_info_for', default=None)
	related_experiment = models.OneToOneField(Experiment, on_delete=models.CASCADE, related_name='scheduling_info_for')
	email_reminder = models.CharField(
			max_length=5,
			choices=EMAIL_REMINDER_CHOICES,
			default= '',
			blank=True,
			verbose_name='Email Reminder Time',
			help_text='If set, we will remind you when your experiment is running so you know when you can expect your results.'
		)
	def __str__(self):
		return 'Email reminder: {eremind} for {title} starting at {start}'.format(eremind=self.get_email_reminder_display(), title=self.related_experiment.experiment_title, start=self.related_event.start.strftime('%x %I:%M %p'))

class Command(models.Model):
	class Meta:
		abstract = True
	parent_experiment = models.ForeignKey(Experiment, on_delete=models.CASCADE)
	delay = models.DecimalField(max_digits=11, decimal_places=6, verbose_name='Command Start Time', help_text='Enter a number, in seconds, with up to 6 decimal places. 0 is the start of the experiment.')
	
class RunResult(models.Model):
	class Meta:
		verbose_name= 'Experiment Run'
		verbose_name_plural= 'Experiment Runs'
	experiment = models.ForeignKey(Experiment, on_delete=models.CASCADE)
	event = models.ForeignKey(Event, on_delete=models.CASCADE)
	log = models.TextField(blank=True, default=None, null=True)

	def get_absolute_url(self):
		return reverse_lazy('display_results', args=[experiment.exp_pk, self.pk])
	def __str__(self):
		return self.experiment.experiment_title + ' at ' + self.event.start.strftime('%x %I:%M %p')
		

class OneTimeSSSCommand(Command):
	class Meta:
		verbose_name='SSS Command'
	COMMAND_CHOICES = [
		('', '--Select Command--'),
		('TurnIgnitionOn(0)', 'Turn Ignition On (none)'),
		('TurnIgnitionOff(0)', 'Turn Ignition Off (none)'),
		('SetAxleBasedVehicleSpeed(1)', 'Set Axle-Based Vehicle Speed (mph)'),
		('SetBrakePressure(2)', 'Set Brake Pressure (0-100%, duration in s)'),
		('EndExperiment(0)', 'End Experiment (none)')
	]
	commandchoice = models.CharField(choices=COMMAND_CHOICES,max_length=256, verbose_name='Command')
	quantity = SeparatedValuesField(blank=True, null=True, default=None, token=',')
	is_repeated = models.BooleanField(default=False, verbose_name='Enable Repetition', help_text='Check this if you want to repeat this command more than once at a regular interval.')
	repeat_delay = models.DecimalField(max_digits=11, decimal_places=6,  blank=True, null=True, verbose_name='Repeat Delay', help_text='Time, in seconds, between the consecutive executions of this command.')
	repeat_count = models.PositiveIntegerField(blank=True, null=True, verbose_name='Execution Count')
	def get_absolute_url(self):
		return reverse('edit_command', kwargs={
			'exp_pk': self.parent_experiment.exp_pk,
			'pk2': self.pk
			})

	def __str__(self):
		if not self.quantity:
			string = self.get_commandchoice_display() + ' command starting at ' + str(self.delay) + ' s'
		else:
			string = self.get_commandchoice_display() + ' command starting at ' + str(self.delay) + ' s with quantity (list) ' + str(self.quantity)
		if self.is_repeated:
			string += ' (repeated ' + str(self.repeat_count) + ' times at a ' + str(self.repeat_delay) + ' second interval)'
		return string
	def clean(self):
		choice = self.commandchoice
		if choice == '':
			raise ValidationError(_('Please choose a command.'))
		argcount = int(choice.split('(')[1][:-1])
		args = self.quantity
		if args is None:
			if argcount != 0:
				raise ValidationError(_('Wrong number of arguments. {commandchoice} takes {argcount} arguments in the "Quantity" field.'.format(commandchoice=self.get_commandchoice_display(),argcount=argcount)))
		elif len(args) != argcount and argcount == 1:
			raise ValidationError(_('Wrong number of arguments. {commandchoice} takes {argcount} argument in the "Quantity" field.'.format(commandchoice=self.get_commandchoice_display(),argcount=argcount)))
		elif len(args) != argcount:
			raise ValidationError(_('Wrong number of arguments. {commandchoice} takes {argcount} arguments in the "Quantity" field.'.format(commandchoice=self.get_commandchoice_display(),argcount=argcount)))



class SPNPGNEntry(models.Model):
	pgn = models.PositiveIntegerField(blank=True,null=True)
	spn = models.PositiveIntegerField(blank=True,null=True)
	pgn_length = models.CharField(max_length=8)
	name = models.CharField(max_length=100)
	spn_length = models.PositiveSmallIntegerField(blank=True,null=True)
	description = models.TextField()
	pgl = models.CharField(max_length=100)
	position = models.CharField(max_length=15)
	transmissionrate_ms = models.CharField(max_length=150)
	units = models.CharField(max_length=20)
	offset = models.CharField(max_length=15)

	def __str__(self):
		return "SPN: {spn} Name: {name}".format(spn=self.spn, name=self.name)

class CANCommand(Command):
	LENGTH_CHOICES = [(0,0),
	(1,1),
	(2,2),
	(3,3),
	(4,4),
	(5,5),
	(6,6),
	(7,7),
	(8,8)]
	INF_CHOICES = [(1, 'can1')]
	length = models.PositiveSmallIntegerField(choices=LENGTH_CHOICES, default=0)
	message = models.CharField(max_length=16, default='0000000000000000')
	message_id = models.CharField(max_length=8, default='00000000')
	interface = models.PositiveSmallIntegerField(choices=INF_CHOICES, default=1)
	is_extended_can = models.BooleanField(default=False)
	def get_absolute_url(self):
		return reverse_lazy('edit_can_command', args=[self.parent_experiment.exp_pk, self.pk])

def with_index(seq):
		for i in range(len(seq)):
			yield i, seq[i]

class ObservableQuantity(models.Model):
	class Meta:
		verbose_name='Observable Quantity'
		verbose_name_plural='Observable Quantities'
	related_experiment = models.ForeignKey(Experiment, on_delete=models.CASCADE)
	xdata = SeparatedValuesField(blank=True, null=True, default=[])
	ydata = SeparatedValuesField(blank=True, null=True, default=[])
	name = models.CharField(blank=True, null=True, default=None, max_length=256)
	entry = models.ForeignKey(SPNPGNEntry, on_delete=models.CASCADE, blank=True, null=True, default=None)
	related_run = models.ForeignKey(RunResult, on_delete=models.CASCADE, blank=True, null=True, default=None)
	plottable = models.BooleanField(default=True)
	

	def gen_plot_html(self):
		if self.xdata is None or self.ydata is None or not self.plottable:
			return ''
		vinlabels = []
		vins = []
		x = self.xdata.split(',')
		for i in range(len(x)):
			x[i] = float(x[i])
		y = self.ydata.split(',')
		if self.entry:
			for i in range(len(y)):
				y[i] = float(y[i])
		elif self.name == 'VIN':
			vincount = 1
			lastelem = bytes.fromhex(y[0]).decode('ascii')
			lastts = x[0]
			vinlabels.append('VIN'+str(vincount)+': ' + lastelem)
			vins.append(lastelem)
			changes = 0
			for i in range(len(y)):
				y[i] = bytes.fromhex(y[i]).decode('ascii')
			newy = y[:]
			for idx, elem in with_index(y):
				if elem != lastelem and elem not in vins:
					newy.insert(idx+changes,lastelem)
					x.insert(idx+changes,x[idx+changes-1])
					vincount += 1
					vinlabels.append('VIN' + str(vincount) + ': ' + elem)
					vins.append(elem)
					changes += 1
				lastelem = elem
			for label in vinlabels:
				for idx, elem in with_index(newy):
					if type(elem) is str and elem in label:
						newy[idx] = int(label[3:4])
			y = newy
			for i in range(len(vins)):
				vins[i] = vins[i].strip(' ')
		allquantities = ObservableQuantity.objects.filter(related_experiment=self.related_experiment)
		idx=0
		for i in range(len(allquantities)):
			if allquantities[i] is self:
				idx=i
				break
		xmax = max(x)
		xmin = min(x)
		ymax = max(y)
		ymin = min(y)
		ymargin = (ymax-ymin) * .3
		xmargin = (xmax-xmin) * .3
		colorstr = CHART_LINE_COLORS[idx % len(CHART_LINE_COLORS)]
		if self.entry:
			if self.entry.spn == 904:
				ytitle = self.entry.name + ' (mph)'
		else:
			ytitle = self.name
		data = [dict(x=x, y=y,
				mode='lines+markers',
				line=dict(width=6,color='#1f3e70'),
				marker=dict(color='#adccff')
				),	
			]
		if self.entry:
			if ObservableQuantity.objects.filter(name='Governor Speed',related_run=self.related_run).exists():
				govq = ObservableQuantity.objects.filter(name='Governor Speed',related_run=self.related_run).first()
				gxdata = govq.xdata.split(',')
				mingx = float(gxdata[0])
				for i in range(len(gxdata)):
					gxdata[i] = float(gxdata[i]) - mingx
				gydata = govq.ydata.split(',')
				for i in range(len(gydata)):
					gydata[i] = float(gydata[i])
				gdata = {
				'x': gxdata,
				'y': gydata,
				'mode': 'lines+markers',
				'line': dict(width=6,color='#701f1f'),
				'marker': dict(color='#f28e8e')
				}
				data.append(gdata)
		layout = dict(height='80%', width='80%', plot_bgcolor='rgba(0,0,0,0)', paper_bgcolor='rgba(0,0,0,0)', showlegend=False,
			font=dict(family='"Helvetica Neue",Helvetica,Arial,sans-serif', color='#c8c8c8'),
			xaxis=dict(title='Time (s)', range=[min(x), max(x) + xmargin], autorange=False, color='#bbb'),
			yaxis=dict(title=ytitle, range=[min(y), max(y) + ymargin], autorange=False, color='#bbb'),
			title=ytitle,
			titlefont=dict(family='"Helvetica Neue",Helvetica,Arial,sans-serif', color='#c8c8c8'),
			 hovermode='closest')
		if self.name and self.name == 'VIN':
			layout['yaxis'].update(dict(tickvals=list(range(1,len(vins) + 1)),ticktext=vins,autorange=True))
		if len(data) > 1:
			layout['yaxis'].update(dict(autorange=True))
			layout.update(dict(showlegend=True))
			data[0].update(dict(name=ytitle))
			data[1].update(dict(name='Governor Speed (mph)'))
		figure = dict(data=data, layout=layout)
		plot_div = plot(figure, output_type='div', show_link=False, include_plotlyjs=True, config=dict(autosizable=True))
		return plot_div

class CANGenCommand(Command):
	INF_CHOICES = [(1, 'can1')]
	interface = models.PositiveSmallIntegerField(choices=INF_CHOICES, default=1)
	gap = models.PositiveIntegerField(blank=True,null=True,default=None, help_text='The gap between consecutive generated commands in ms.  If left blank, implies a 200ms gap.  The gap can be 0 ms to send each message ASAP.')
	generate_extended_can = models.BooleanField(default=False)
	send_rtr_frame = models.BooleanField(default=False)
	message_length = models.CharField(max_length=1,blank=True,null=True, help_text='Valid values: 0-8, blank (random) or i, which loops the length from 0 to 8.')
	message_id = models.CharField(max_length=8,blank=True,null=True, help_text='Valid values: any 4-byte (8-character) hexidecimal message id.  No 0x prefix.  If left blank will be random.')
	can_data = models.CharField(max_length=16,blank=True,null=True, help_text='Valid values: any 8-byte hexidecimal message, blank (random), or i, which sends messages of increasing value until it hits the maximum, then repeats until the command is over.')
	number_of_can_frames_before_end = models.PositiveIntegerField(blank=True, null=True, help_text='Terminates after the specified number of sent/recieved CAN frames.  Leave blank for until the experiment ends.')

	def get_absolute_url(self):
		return reverse_lazy('edit_cangen_command', args=[self.parent_experiment.exp_pk, self.pk])

class ECUUpdate(Command):
	class Meta:
		verbose_name = 'ECU Update'
	UPDATE_CHOICES = [(1, 'VIN Update'),(2, 'Governor Speed Update')]
	update_type = models.PositiveSmallIntegerField(choices=UPDATE_CHOICES, default=1)
	vin = models.CharField(blank=True,null=True,default=None,max_length=17)
	governor_speed = models.FloatField(default=None,blank=True,null=True)

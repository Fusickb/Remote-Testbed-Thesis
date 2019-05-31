from . import models
from schedule.models import Event, Calendar
from django import forms	
from django.forms import ModelForm, Form, MultiWidget
from django.core.exceptions import ValidationError
from schedule.forms import SpanForm
from datetime import timedelta, datetime
import arrow
from django.utils.translation import ugettext_lazy as _
from . import tasks
from trucksite import settings
from notifications.signals import notify
from django.db.models import Q


class MultiHexEightByteField(forms.MultiValueField):
	def compress(self, data_list):
		if data_list:
			return ''.join(data_list)
		return None
class MultiHexFourByteField(forms.MultiValueField):
	def compress(self, data_list):
		if data_list:
			return ''.join(data_list)
		return None
class MultiHexEightByteWidget(forms.MultiWidget):
	def __init__(self, attrs=None, mode=0):
		_widgets = tuple([forms.TextInput() for i in range(8)])
		super(MultiHexEightByteWidget, self).__init__(_widgets, attrs)
	
	def decompress(self, value):
		if value:
			bytestrlist = []
			for i in range(0,16,2):
				bytestrlist.append(value[i:i+2])
			return bytestrlist
		else:
			return [None for i in range(8)]

class MultiHexFourByteWidget(forms.MultiWidget):
	def __init__(self, attrs=None, mode=0):
		_widgets = tuple([forms.TextInput() for i in range(4)])
		super(MultiHexFourByteWidget, self).__init__(_widgets, attrs)

	def decompress(self, value):
		if value:
			bytestrlist = []
			for i in range(0,8,2):
				bytestrlist.append(value[i:i+2])
			return bytestrlist
		else:
			return [None for i in range(4)]

class ExperimentForm(ModelForm):
	class Meta:
		model = models.Experiment
		exclude = ['created_by', 'experiment_created_date', 'is_scheduled']

class DuplicateExperimentForm(ModelForm):
	class Meta:
		model = models.Experiment
		exclude = ['created_by', 'experiment_created_date', 'is_scheduled', 'experiment_title']
	new_title = forms.CharField(required=False, help_text='If left blank, will append "Copy" to the tile of the experiment to be copied.')
	dup_experiment = forms.ModelChoiceField(queryset=models.Experiment.objects.all(), required=True, empty_label='--Select an Experiment--', label='Target Experiment')
	
	def clean(self):
		cleaned_data = super(DuplicateExperimentForm, self).clean()
		if cleaned_data['new_title'] == '' or cleaned_data['new_title'] == cleaned_data['dup_experiment'].experiment_title:
			cleaned_data['new_title'] = cleaned_data['dup_experiment'].experiment_title + ' Copy'
		cleaned_data['created_by'] = cleaned_data['dup_experiment'].created_by
		return cleaned_data

	def save(self, commit=True):
		new_exp = models.Experiment.objects.create(experiment_title=self.cleaned_data['new_title'],created_by=self.cleaned_data['created_by'])
		commands = self.cleaned_data['dup_experiment'].get_all_commands_list()
		for command in commands:
			if isinstance(command, models.OneTimeSSSCommand):
		 		models.OneTimeSSSCommand.objects.create(quantity=command.quantity, is_repeated=command.is_repeated, repeat_delay=command.repeat_delay, delay=command.delay, commandchoice=command.commandchoice, parent_experiment=new_exp)
			elif isinstance(command, models.CANCommand):
				models.CANCommand.objects.create(delay=command.delay, parent_experiment= new_exp, message=command.message, message_id=command.message_id, message_length=command.message_length, interface=command.interface, is_extended_can=command.is_extended_can)
			elif isinstance(command, models.CANGenCommand):
				models.CANGenCommand.objects.create(delay=command.delay, parent_experiment=new_exp, interface=command.interface, gap=command.gap, generate_extended_can=command.generate_extended_can, send_rtr_frame=command.send_rtr_frame, message_id=command.message_id, can_data=command.can_data, number_of_can_frames_before_end=command.number_of_can_frames_before_end)



class SchedulingInfoForm(ModelForm):
	class Meta:
		model = models.ExperimentSchedulingInfo
		exclude = ['related_experiment','related_event']

class SSSCommandForm(ModelForm):
	class Meta:
		model = models.OneTimeSSSCommand
		fields = '__all__'
	quantity = forms.CharField(required=False, widget=forms.TextInput(), help_text='Note the units in your command choice and comma-separate values if there are multiple units specified (omit percent signs from percents).')
	is_repeated = forms.BooleanField(required=False, widget=forms.CheckboxInput(attrs={'id': 'check'}),help_text='Check this if you want to repeat this command more than once at a regular interval. Leave unchecked otherwise.')
	repeat_delay = forms.IntegerField(required=False, widget=forms.TextInput(attrs={'class': 'repeatonly', 'disabled': ''}),help_text='The amount of time, in seconds, between each instance of this command.')
	repeat_count = forms.IntegerField(required=False, widget=forms.TextInput(attrs={'class': 'repeatonly', 'disabled': ''}),help_text='The first instance of this command starts on the time you specify the command to start.')
	parent_experiment = forms.ModelChoiceField(queryset=models.Experiment.objects.all(), required=True, widget=forms.HiddenInput())
	def clean(self):
		cleaned_data = super(SSSCommandForm, self).clean()
		if cleaned_data['is_repeated'] and (cleaned_data.get('repeat_delay') is None or cleaned_data.get('repeat_count') is None):
			raise ValidationError(_('One of the fields that is required for a repeated command is blank.'))
		if cleaned_data['commandchoice'].split('(')[0] == 'EndExperiment' and cleaned_data['is_repeated']:
			raise ValdiationError(_('Cannot loop the "End Experiment" command.'))
		if cleaned_data['commandchoice'] == 'EndExperiment(0)' and models.OneTimeSSSCommand.objects.filter(parent_experiment=cleaned_data['parent_experiment'], commandchoice=cleaned_data['commandchoice']).exists():
			raise ValidationError(_('Cannot have more than one "End Experiment" command at a time.'))
		expcommands = models.OneTimeSSSCommand.objects.filter(Q(parent_experiment = self.cleaned_data['parent_experiment']), ~Q(commandchoice='EndExperiment(0)')).order_by('delay')
		delay = self.cleaned_data['delay']
		problemdelay = None
		for command in expcommands:
			if command.delay > delay:
				problemdelay = command.delay
		if problemdelay is not None:
			raise ValidationError(_("Command(s) exist after the current end of the experiment.  Please delete those commands or increase the start time on the end experiment command past {delay} s.".format(delay=problemdelay)))
		return cleaned_data

class CANCommandForm(ModelForm):
	class Meta:
		model = models.CANCommand
		fields = '__all__'
	parent_experiment = forms.ModelChoiceField(queryset=models.Experiment.objects.all(), required=True, widget=forms.HiddenInput())
	message = MultiHexEightByteField(fields=[forms.RegexField(regex=r'^[0-9a-fA-F]{2}$', strip=True) for i in range(8)], help_text='Each box is a byte (00-FF), leading zeroes required.', widget=MultiHexEightByteWidget(attrs={'class': 'form-control eight-byte', 'placeholder': ''}))
	message_id = MultiHexFourByteField(fields=[forms.RegexField(regex=r'^[0-9a-fA-F]{2}$', strip=True) for i  in range(4)], help_text='The message ID, from MSB to LSB.  All bytes are required.', widget=MultiHexFourByteWidget(attrs={'class': 'form-control four-byte', 'placeholder': ''}))
	interface = forms.ChoiceField(choices=[(1, 'can1')], widget=forms.Select(attrs={'class': 'form-control disabled'}))
class CANGenCommandForm(ModelForm):
	class Meta:
		model = models.CANGenCommand
		fields = '__all__'
		labels = {
			"generate_extended_can": "Generate Extended CAN",
			"send_rtr_frame": "Send RTR Frame",
			"message_id": "Message ID",
			"can_data": "CAN Data",
			"number_of_can_frames_before_end": "Number of CAN Frames Before End"
		}
	parent_experiment = forms.ModelChoiceField(queryset=models.Experiment.objects.all(), required=True, widget=forms.HiddenInput())
	message_length = forms.RegexField(regex=r'^[0-8i]{1}$', strip=True, required=False)
	message_id = forms.RegexField(regex=r'(^[0-9A-Fa-f]{8}$)|(^i$)', strip=True, required=False)
	can_data = forms.RegexField(regex=r'(^[0-9A-Fa-f]{16}$)|(^i$)', strip=True, required=False)
	interface = forms.ChoiceField(choices=[(1, 'can1')], widget=forms.Select(attrs={'class': 'form-control disabled'}))

class ExperimentEventForm(ModelForm):
	start = forms.SplitDateTimeField(label='Start', help_text=_('Format example: 2017-03-14 09:30:00 AM'), input_date_formats=['%Y-%m-%d'], input_time_formats=['%I:%M:%S %p'], widget=forms.SplitDateTimeWidget(time_format="%I:%M:%S %p"))
	end = forms.SplitDateTimeField(label='End', help_text=_('Maximum Duration: 4 Hours'), input_date_formats=['%Y-%m-%d'], input_time_formats=['%I:%M:%S %p'], widget=forms.SplitDateTimeWidget(time_format="%I:%M:%S %p"))
	email_reminder = forms.ChoiceField(choices=models.EMAIL_REMINDER_CHOICES, required=False)
	exp_pk = forms.IntegerField(widget=forms.HiddenInput())
	tzoffset = forms.FloatField(widget=forms.TextInput(attrs={'type': 'hidden'})) #to get current user timezone so scheduling timezone issues go away
	class Meta:
		model = Event
		fields = ['start','end','email_reminder','exp_pk']
	def clean(self):
		cleaned_data = super(ExperimentEventForm, self).clean()
		utctzoffset = float(self['tzoffset'].data) * -1
		cleaned_data['utctzoffset'] = utctzoffset
		utctzoffsetstr = ''
		if utctzoffset >= 0:
			utctzoffsetstr += '+'
		else:
			utctzoffsetstr += '-'
		absutcoffset = abs(utctzoffset)
		utctzoffsetstr += '{:>02}:'.format(int(absutcoffset))
		if utctzoffset - int(utctzoffset) != 0:
			utctzoffsetstr += '30'
		else:
			utctzoffsetstr += '00'
		startdata = self['start'].data
		startdata.append(utctzoffsetstr)
		enddata = self['end'].data
		enddata.append(utctzoffsetstr)
		naivestart=arrow.get(' '.join(startdata), 'YYYY-MM-DD hh:mm:ss A ZZ')
		naiveend=arrow.get(' '.join(enddata), 'YYYY-MM-DD hh:mm:ss A ZZ')
		start = naivestart.to('US/Central').datetime
		end = naiveend.to('US/Central').datetime
		#Check for duration
		if end-start > timedelta(hours=4):
			raise forms.ValidationError(_('Duration longer than 4 hours.'))
		#check for conflicts, +15 seconds for processing and 30 seconds before starting for initialization
		conflict_queryset1 = models.Event.objects.filter(start__lte=end+timedelta(seconds=15),start__gte=start-timedelta(seconds=30))
		conflict_queryset2 = models.Event.objects.filter(end__gte=start-timedelta(seconds=30),end__lte=end+timedelta(seconds=15))
		if conflict_queryset1.exists() or conflict_queryset2.exists():
			raise forms.ValidationError(_("This experiment conflicts with another's time slot."))
		self.cleaned_data.update(start=start, end=end, tzoffset=utctzoffsetstr)
		return self.cleaned_data
	def save(self, commit=True):
		evt = super(ExperimentEventForm, self).save(commit=False)
		exp = models.Experiment.objects.get(exp_pk=self.cleaned_data['exp_pk'])
		evt.start = self.cleaned_data['start']
		runtime = evt.start - timedelta(seconds=30)
		evt.end = self.cleaned_data['end']
		evt.creator = exp.created_by
		evt.title = exp.experiment_title
		evt.calendar = Calendar.objects.get(slug='experiment-calendar')
		if commit:
			evt.description = ''
			evt.color_event = '#ff00ff' 
			evt.save()
			esi = models.ExperimentSchedulingInfo.objects.create(related_event=evt,related_experiment=exp)
			reminder_time = None
			if esi.email_reminder == '10min':
				td = timedelta(minutes=-10)
			elif esi.email_reminder == '15min':
				td = timedelta(minutes=-15)
			elif esi.email_reminder == '30min':
				td = timedelta(minutes=-30)
			elif esi.email_reminder == '1hr':
				td = timedelta(hours=-1)
			elif esi.email_reminder == '1day':
				td = timedelta(days=-1)
			else:
				td = timedelta()
			if esi.email_reminder != '':
				reminder_time = runtime + td
			if reminder_time is not None and reminder_time > timezone.now():
				tasks.send_email_reminder(exp.exp_pk, schedule=reminder_time)
			tasks.send_experiment_json(exp.exp_pk, schedule=runtime)
			notify.send(sender=exp, actor=exp.created_by, recipient=exp.created_by, verb="was just scheduled for " + arrow.get(evt.start).replace(hours=-float(self.cleaned_data['utctzoffset'])).datetime.strftime('%x %I:%M:%S %p'),	 action_object=exp, public=False, level='info')
			newrun = models.RunResult.objects.create(event=evt, experiment=exp)
			mostrecentrun = None
			if models.RunResult.objects.filter(experiment=exp).exists() and models.ObservableQuantity.objects.filter(Q(related_experiment=exp), ~Q(related_run=None)):
				mostrecentrun = models.RunResult.objects.filter(experiment=exp).latest('event__end')
				for quantity in models.ObservableQuantity.objects.filter(related_experiment=exp, related_run=mostrecentrun):
					quantity.pk = None
					quantity.related_run = newrun
					quantity.related_experiment = exp
					quantity.xdata = ''
					quantity.ydata = ''
					quantity.save()
			else:
				for quantity in models.ObservableQuantity.objects.filter(related_experiment=exp, related_run=None):
					quantity.pk = None
					quantity.related_run = newrun
					quantity.related_experiment = exp
					quantity.xdata = ''
					quantity.ydata = ''
					quantity.save()

		return evt

GRAPHABLE_QUANTITY_CHOICES = [
(904, 'Axle-Based Vehicle Speed'),
] #add to this

class GraphableQuantityChoiceForm(forms.Form):
	quantities = forms.MultipleChoiceField(choices=GRAPHABLE_QUANTITY_CHOICES, widget=forms.CheckboxSelectMultiple, required=False)
	
class ECUUpdateForm(ModelForm):
	class Meta:
		model = models.ECUUpdate
		fields = '__all__'
	parent_experiment = forms.ModelChoiceField(queryset=models.Experiment.objects.all(), required=True, widget=forms.HiddenInput())
	vin = forms.RegexField(regex=r'[\x00-\x7F]+',strip=True,label='VIN', required=False)
	governor_speed = forms.FloatField(min_value=0.0,max_value=200.0, required=False, widget=forms.NumberInput(attrs={'disabled': ''}),label='Governor Speed (mph)')
	def clean(self):
		cleaned_data = super(ECUUpdateForm, self).clean()
		if cleaned_data['update_type'] == 1 and cleaned_data['vin'] is None:
			raise ValidationError('VIN Update selected with no VIN entered.') 
		elif cleaned_data['update_type'] == 2 and cleaned_data['governor_speed'] is None:
			raise ValidationError('Governor Speed Update selected with no Governor Speed.')
		return cleaned_data




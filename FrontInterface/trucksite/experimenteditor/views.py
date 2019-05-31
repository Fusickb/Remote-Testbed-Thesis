from django.shortcuts import get_object_or_404
from django.views.generic import TemplateView
from django.views.generic.edit import CreateView, DeleteView, UpdateView, FormView, ModelFormMixin 
from django.views.generic.list import ListView
from django.contrib.auth.mixins import LoginRequiredMixin, UserPassesTestMixin
from plotly.offline.offline	 import _plot_html
from django.urls import reverse_lazy, resolve
from django.utils.six.moves.urllib.parse import urlparse
from . import models
from . import forms
import numpy as np
from schedule.views import CalendarByPeriodsView
from schedule.periods import Week
from schedule.models import Calendar, Event
from schedule.utils import coerce_date_dict
from . import tasks
from datetime import timedelta, datetime
from django.utils import timezone
from django.http import HttpResponse
import struct
import os
from plotly.offline import plot

GLOBAL_CALENDAR_SLUG = 'experiment-calendar'
LOG_DIRECTORY = 'testbedLogs/'
CHART_LINE_COLORS = ['hsl(164.7, 100%, 36.9%)', 'hsl(0, 100%, 36.9%)', 'hsl(253, 100%, 36.9%)', 'hsl(31, 100%, 36.9%)' 'hsl(281, 100%, 36.9%)', 'hsl(76, 100%, 36.9%)']

class RunResultListView(LoginRequiredMixin, ListView):
	model = models.RunResult
	template_name = 'experimenteditor/results_list.html'
	def get_queryset(self):
		resolveobj = resolve(self.request.path)
		exp_pk = resolveobj.kwargs.get('exp_pk')
		return models.RunResult.objects.filter(experiment__exp_pk=exp_pk).order_by('-event__start')
	def get_context_data(self, *args, **kwargs):
		context_data = super(RunResultListView, self).get_context_data()
		context_data['experiment'] = models.Experiment.objects.get(exp_pk=resolve(self.request.path).kwargs['exp_pk'])
		return context_data


class RunResultDeleteView(LoginRequiredMixin, DeleteView):
	model = models.RunResult
	pk_url_kwarg = 'runpk'
	def get_success_url():
		resolveobj = resolve(self.request.path)
		exp_pk = resolveobj.kwargs.get('exp_pk')
		return reverse_lazy('results_list', args=[exp_pk])
	def get_context_data(self, *args, **kwargs):
		context_data = super(RunResultDeleteView, self).get_context_data()
		context_data['experiment'] = models.Experiment.objects.get(exp_pk=resolve(self.request.path).kwargs['exp_pk'])
		return context_data

class MyExperimentsView(LoginRequiredMixin, TemplateView):
	template_name='experimenteditor/myexperiments.html'
	def get_context_data(self, *args, **kwargs):
		context = super(MyExperimentsView, self).get_context_data(*args, **kwargs)
		context['experiments'] = models.Experiment.objects.filter(created_by=self.request.user).order_by('experiment_created_date')
		return context

class CreateExperimentView(LoginRequiredMixin, CreateView):
	model = models.Experiment
	form_class = forms.ExperimentForm

	def form_valid(self, form):
		form.instance.created_by = self.request.user
		return super(CreateExperimentView, self).form_valid(form)

class DuplicateExperimentView(LoginRequiredMixin, CreateView):
	model = models.Experiment
	form_class = forms.DuplicateExperimentForm
	template_name = 'experimenteditor/experiment_duplicate.html'

	def get_success_url(self):
		return reverse_lazy('my_experiments')

class EditExperimentView(LoginRequiredMixin, UserPassesTestMixin, TemplateView):
	template_name = 'experimenteditor/experiment_edit.html'
	def test_func(self):
		user_experiments = models.Experiment.objects.filter(created_by=self.request.user)
		resolveobj = resolve(self.request.path)
		experiment = models.Experiment.objects.get(exp_pk=resolveobj.kwargs.get('exp_pk'))
		return experiment in user_experiments

	def get_context_data(self, *args, **kwargs):
		context = super(EditExperimentView, self).get_context_data(*args, **kwargs)
		context['commands'] = models.OneTimeSSSCommand.objects.filter(parent_experiment__exp_pk=kwargs.get('exp_pk')).order_by('delay')
		context['cancommands'] = models.CANCommand.objects.filter(parent_experiment__exp_pk=kwargs.get('exp_pk')).order_by('delay')
		context['cangencommands'] = models.CANGenCommand.objects.filter(parent_experiment__exp_pk=kwargs.get('exp_pk')).order_by('delay')
		context['ecuupdates'] = models.ECUUpdate.objects.filter(parent_experiment__exp_pk=kwargs.get('exp_pk')).order_by('delay')
		context['experiment'] = models.Experiment.objects.get(exp_pk=kwargs.get('exp_pk'))
		return context

class ConfirmDeleteExperimentView(LoginRequiredMixin, DeleteView):
	model = models.Experiment
	success_url = reverse_lazy('my_experiments')
	template_name = 'experimenteditor/experiment_delete.html'	
	pk_url_kwarg = 'exp_pk'

class CreateCommandView(LoginRequiredMixin, CreateView):
	model = models.OneTimeSSSCommand
	form_class = forms.SSSCommandForm
	template_name = 'experimenteditor/ssscommand_form.html' 
	
	def get_initial(self):
		initial=super(CreateCommandView, self).get_initial()
		initial['parent_experiment'] = models.Experiment.objects.get(exp_pk=resolve(self.request.path).kwargs['exp_pk'])
		return initial

	def form_valid(self, form):
		path = self.request.path
		exp_pk = resolve(path).kwargs['exp_pk']
		form.instance.parent_experiment = models.Experiment.objects.get(pk=exp_pk)
		self.success_url = reverse_lazy('edit_experiment', args=[exp_pk])
		return super(CreateCommandView, self).form_valid(form)
	
	def get_context_data(self, *args, **kwargs):
		context = super(CreateCommandView, self).get_context_data(*args, **kwargs)
		context['experiment'] = models.Experiment.objects.get(exp_pk=resolve(self.request.path).kwargs.get('exp_pk'))
		return context 

class DeleteCommandView(LoginRequiredMixin, DeleteView):
	model = models.OneTimeSSSCommand
	pk_url_kwarg = 'pk2'
	template_name = 'experimenteditor/delete_command.html'
	def get_context_data(self, *args, **kwargs):
		context = super(DeleteCommandView, self).get_context_data(*args, **kwargs)
		resolveobj = resolve(self.request.path)
		context['experiment'] = models.Experiment.objects.get(exp_pk=resolveobj.kwargs['exp_pk'])
		return context
	def get_success_url(self):
		resolveobj = resolve(self.request.path)
		return reverse_lazy('edit_experiment', kwargs={'exp_pk': resolveobj.kwargs['exp_pk']})


class UpdateCommandView(LoginRequiredMixin, UpdateView):
	model = models.OneTimeSSSCommand
	form_class = forms.SSSCommandForm
	template_name = 'experimenteditor/ssscommand_form.html'
	
	def get_initial(self):
		initial=super(UpdateCommandView, self).get_initial()
		initial['parent_experiment'] = models.Experiment.objects.get(exp_pk=resolve(self.request.path).kwargs['exp_pk'])
		return initial

	def get_object(self, queryset=None):
		commandpk = resolve(self.request.path).kwargs['pk2']
		object_get = get_object_or_404(self.model, pk=commandpk)
		return object_get

	def form_valid(self, form):
		path = self.request.path
		exp_pk = resolve(path).kwargs['exp_pk']
		form.instance.parent_experiment = models.Experiment.objects.get(pk=exp_pk)
		self.success_url = reverse_lazy('edit_experiment', args=[exp_pk])
		return super(UpdateCommandView, self).form_valid(form)

	def get_context_data(self, *args, **kwargs):
		context = super(UpdateCommandView, self).get_context_data(*args, **kwargs)
		context['experiment'] = models.Experiment.objects.get(exp_pk=resolve(self.request.path).kwargs.get('exp_pk'))
		return context 

class CreateCanCommandView(LoginRequiredMixin, CreateView):
	model = models.CANCommand
	form_class = forms.CANCommandForm
	template_name = 'experimenteditor/cancommand_form.html'

	def get_initial(self):
		initial=super(CreateCanCommandView, self).get_initial()
		initial = initial.copy()
		initial['parent_experiment'] = models.Experiment.objects.get(exp_pk=resolve(self.request.path).kwargs['exp_pk'])
		initial['message'] = '0000000000000000'
		initial['message_id'] = '00000000'
		return initial
	
	def form_valid(self, form):
		path = self.request.path
		exp_pk = resolve(path).kwargs['exp_pk']
		form.instance.parent_experiment = models.Experiment.objects.get(pk=exp_pk)
		self.success_url = reverse_lazy('edit_experiment', args=[exp_pk])
		return super(CreateCanCommandView, self).form_valid(form)


	def get_context_data(self, *args, **kwargs):
		context = super(CreateCanCommandView, self).get_context_data(*args, **kwargs)
		context['experiment'] = models.Experiment.objects.get(exp_pk=resolve(self.request.path).kwargs.get('exp_pk'))
		return context

class UpdateCanCommandView(LoginRequiredMixin, UpdateView):
	model = models.CANCommand
	form_class = forms.CANCommandForm
	template_name = 'experimenteditor/cancommand_form.html'
	
	def get_initial(self):
		initial=super(UpdateCanCommandView, self).get_initial()
		initial['parent_experiment'] = models.Experiment.objects.get(exp_pk=resolve(self.request.path).kwargs['exp_pk'])
		return initial

	def get_object(self, queryset=None):
		commandpk = resolve(self.request.path).kwargs['pk2']
		object_get = get_object_or_404(self.model, pk=commandpk)
		return object_get

	def form_valid(self, form):
		path = self.request.path
		exp_pk = resolve(path).kwargs['exp_pk']
		form.instance.parent_experiment = models.Experiment.objects.get(pk=exp_pk)
		self.success_url = reverse_lazy('edit_experiment', args=[exp_pk])
		return super(UpdateCanCommandView, self).form_valid(form)

	def get_context_data(self, *args, **kwargs):
		context = super(UpdateCanCommandView, self).get_context_data(*args, **kwargs)
		context['experiment'] = models.Experiment.objects.get(exp_pk=resolve(self.request.path).kwargs.get('exp_pk'))
		return context 

class DeleteCanCommandView(LoginRequiredMixin, DeleteView):
	model = models.CANCommand
	pk_url_kwarg = 'pk2'
	template_name = 'experimenteditor/delete_can_command.html'
	def get_context_data(self, *args, **kwargs):
		context = super(DeleteCanCommandView, self).get_context_data(*args, **kwargs)
		resolveobj = resolve(self.request.path)
		context['experiment'] = models.Experiment.objects.get(exp_pk=resolveobj.kwargs['exp_pk'])
		return context
	def get_success_url(self):
		resolveobj = resolve(self.request.path)
		return reverse_lazy('edit_experiment', kwargs={'exp_pk': resolveobj.kwargs['exp_pk']})

class CreateCanGenCommandView(LoginRequiredMixin, CreateView):
	model = models.CANGenCommand
	form_class = forms.CANGenCommandForm
	template_name = 'experimenteditor/cangencommand_form.html'

	def get_initial(self):
		initial=super(CreateCanGenCommandView, self).get_initial()
		initial = initial.copy()
		initial['parent_experiment'] = models.Experiment.objects.get(exp_pk=resolve(self.request.path).kwargs['exp_pk'])
		return initial
	
	def form_valid(self, form):
		path = self.request.path
		exp_pk = resolve(path).kwargs['exp_pk']
		form.instance.parent_experiment = models.Experiment.objects.get(pk=exp_pk)
		self.success_url = reverse_lazy('edit_experiment', args=[exp_pk])
		return super(CreateCanGenCommandView, self).form_valid(form)


	def get_context_data(self, *args, **kwargs):
		context = super(CreateCanGenCommandView, self).get_context_data(*args, **kwargs)
		context['experiment'] = models.Experiment.objects.get(exp_pk=resolve(self.request.path).kwargs.get('exp_pk'))
		return context

class UpdateCanGenCommandView(LoginRequiredMixin, UpdateView):
	model = models.CANGenCommand
	form_class = forms.CANGenCommandForm
	template_name = 'experimenteditor/cangencommand_form.html'
	
	def get_initial(self):
		initial=super(UpdateCanGenCommandView, self).get_initial()
		initial['parent_experiment'] = models.Experiment.objects.get(exp_pk=resolve(self.request.path).kwargs['exp_pk'])
		return initial

	def get_object(self, queryset=None):
		commandpk = resolve(self.request.path).kwargs['pk2']
		object_get = get_object_or_404(self.model, pk=commandpk)
		return object_get

	def form_valid(self, form):
		path = self.request.path
		exp_pk = resolve(path).kwargs['exp_pk']
		form.instance.parent_experiment = models.Experiment.objects.get(pk=exp_pk)
		self.success_url = reverse_lazy('edit_experiment', args=[exp_pk])
		return super(UpdateCanGenCommandView, self).form_valid(form)

	def get_context_data(self, *args, **kwargs):
		context = super(UpdateCanGenCommandView, self).get_context_data(*args, **kwargs)
		context['experiment'] = models.Experiment.objects.get(exp_pk=resolve(self.request.path).kwargs.get('exp_pk'))
		return context 

class DeleteCanGenCommandView(LoginRequiredMixin, DeleteView):
	model = models.CANCommand
	pk_url_kwarg = 'pk2'
	template_name = 'experimenteditor/delete_cangen_command.html'
	def get_context_data(self, *args, **kwargs):
		context = super(DeleteCanGenCommandView, self).get_context_data(*args, **kwargs)
		resolveobj = resolve(self.request.path)
		context['experiment'] = models.Experiment.objects.get(exp_pk=resolveobj.kwargs['exp_pk'])
		return context
	def get_success_url(self):
		resolveobj = resolve(self.request.path)
		return reverse_lazy('edit_experiment', kwargs={'exp_pk': resolveobj.kwargs['exp_pk']})

class CreateECUUpdateView(LoginRequiredMixin, CreateView):
	model = models.ECUUpdate
	form_class = forms.ECUUpdateForm
	template_name = 'experimenteditor/ecuupdate_form.html'

	def get_initial(self):
		initial=super(CreateECUUpdateView, self).get_initial()
		initial = initial.copy()
		initial['parent_experiment'] = models.Experiment.objects.get(exp_pk=resolve(self.request.path).kwargs['exp_pk'])
		return initial
	
	def form_valid(self, form):
		path = self.request.path
		exp_pk = resolve(path).kwargs['exp_pk']
		form.instance.parent_experiment = models.Experiment.objects.get(pk=exp_pk)
		self.success_url = reverse_lazy('edit_experiment', args=[exp_pk])
		return super(CreateECUUpdateView, self).form_valid(form)


	def get_context_data(self, *args, **kwargs):
		context = super(CreateECUUpdateView, self).get_context_data(*args, **kwargs)
		context['experiment'] = models.Experiment.objects.get(exp_pk=resolve(self.request.path).kwargs.get('exp_pk'))
		return context

class UpdateECUUpdateView(LoginRequiredMixin, UpdateView):
	model = models.ECUUpdate
	form_class = forms.ECUUpdateForm
	template_name = 'experimenteditor/ecuupdate_form.html'
	
	def get_initial(self):
		initial=super(UpdateECUUpdateView, self).get_initial()
		initial['parent_experiment'] = models.Experiment.objects.get(exp_pk=resolve(self.request.path).kwargs['exp_pk'])
		return initial

	def get_object(self, queryset=None):
		commandpk = resolve(self.request.path).kwargs['pk2']
		object_get = get_object_or_404(self.model, pk=commandpk)
		return object_get

	def form_valid(self, form):
		path = self.request.path
		exp_pk = resolve(path).kwargs['exp_pk']
		form.instance.parent_experiment = models.Experiment.objects.get(pk=exp_pk)
		self.success_url = reverse_lazy('edit_experiment', args=[exp_pk])
		return super(UpdateECUUpdateView, self).form_valid(form)

	def get_context_data(self, *args, **kwargs):
		context = super(UpdateECUUpdateView, self).get_context_data(*args, **kwargs)
		context['experiment'] = models.Experiment.objects.get(exp_pk=resolve(self.request.path).kwargs.get('exp_pk'))
		return context 

class DeleteECUUpdateView(LoginRequiredMixin, DeleteView):
	model = models.ECUUpdate
	pk_url_kwarg = 'pk2'
	template_name = 'experimenteditor/delete_ecu_update.html'
	def get_context_data(self, *args, **kwargs):
		context = super(DeleteECUUpdateView, self).get_context_data(*args, **kwargs)
		resolveobj = resolve(self.request.path)
		context['experiment'] = models.Experiment.objects.get(exp_pk=resolveobj.kwargs['exp_pk'])
		return context
	def get_success_url(self):
		resolveobj = resolve(self.request.path)
		return reverse_lazy('edit_experiment', kwargs={'exp_pk': resolveobj.kwargs['exp_pk']})

class ScheduleExperimentWeekView(LoginRequiredMixin, CalendarByPeriodsView):
	template_name = 'experimenteditor/calendar_week.html'
	object = Calendar.objects.get(slug=GLOBAL_CALENDAR_SLUG)
	def get_context_data(self, *args, **kwargs):	
		context = super(ScheduleExperimentWeekView, self).get_context_data(**kwargs)
		resolveobj = resolve(self.request.path)
		context.update(experiment=models.Experiment.objects.get(exp_pk=resolveobj.kwargs.get('exp_pk')),slug=GLOBAL_CALENDAR_SLUG)
		return context

class ScheduleExperimentFormView(LoginRequiredMixin, CreateView):
	template_name = 'experimenteditor/schedule_experiment.html'
	form_class = forms.ExperimentEventForm
	success_url = reverse_lazy('my_experiments')
	def get_initial(self):
		initial = super(ScheduleExperimentFormView, self).get_initial()
		initial = initial.copy()
		exp_pk = resolve(self.request.path).kwargs['exp_pk']
		initial.update(exp_pk=int(exp_pk))
		date = coerce_date_dict(self.request.GET)
		if date:
			try:
				start = datetime(**date)
				endseconds = models.OneTimeSSSCommand.objects.filter(commandchoice='EndExperiment(0)', parent_experiment__exp_pk=exp_pk).first().delay
				initial.update(start=start, end=start+timedelta(seconds=int(endseconds)))
			except TypeError:
				raise Http404
			except ValueError:
				raise Http404
		calendar = Calendar.objects.get(slug=GLOBAL_CALENDAR_SLUG)
		return initial
	def get_context_data(self, *args, **kwargs):
		context = super(ScheduleExperimentFormView, self).get_context_data(**kwargs)
		resolveobj = resolve(self.request.path)
		context.update(experiment=models.Experiment.objects.get(exp_pk=resolveobj.kwargs.get('exp_pk')))
		return context
	def form_valid(self, form):
		resolveobj = resolve(self.request.path)
		experiment = models.Experiment.objects.get(exp_pk=resolveobj.kwargs.get('exp_pk'))
		resultsurl = '' #str(self.request.get_host()) + str(experiment.get_results_url())
		tasks.send_complete_mail(resolveobj.kwargs.get('exp_pk'), resultsurl, schedule=form.cleaned_data['end'] + timedelta(seconds=10))
		return super(ScheduleExperimentFormView, self).form_valid(form)

class EditGraphableQuantitiesView(LoginRequiredMixin, FormView):
	template_name = "experimenteditor/quantity_edit.html"
	form_class = forms.GraphableQuantityChoiceForm
	success_url = reverse_lazy('my_experiments')
	def get_initial(self):
		initial = super(EditGraphableQuantitiesView, self).get_initial()
		initial = initial.copy()
		resolveobj = resolve(self.request.path)
		exp = models.Experiment.objects.get(exp_pk=resolveobj.kwargs.get('exp_pk'))
		quantities = models.ObservableQuantity.objects.filter(related_experiment=exp)
		if not quantities:
			return initial
		for quantity in quantities:
			if initial.get(quantities) is None:
				initial['quantities'] = []
			if quantity.entry:
				initial['quantities'].append(quantity.entry.spn)
		return initial


	def form_valid(self, form):
		quantities = form.cleaned_data['quantities']
		resolveobj = resolve(self.request.path)
		exp = models.Experiment.objects.get(exp_pk=resolveobj.kwargs.get('exp_pk'))
		oldquantities = models.ObservableQuantity.objects.filter(related_experiment=exp)
		for quantity in oldquantities:
			if quantity.entry:
				if quantity.entry.spn not in quantities:
					quantity.delete()	
		for quantity in quantities:
			if not oldquantities.filter(entry__spn=quantity).exists():
				entry = models.SPNPGNEntry.objects.get(spn=quantity)
				models.ObservableQuantity.objects.create(related_experiment=exp, entry=entry)
		return super(EditGraphableQuantitiesView, self).form_valid(form)

class DisplayResultsView(LoginRequiredMixin, TemplateView):
	template_name = 'experimenteditor/quantity_plot_view.html'
	
	def with_index(self, seq):
		for i in range(len(seq)):
			yield i, seq[i]

	def gen_plot_html(self, xdata, ydata, entryName, xdata2, ydata2):
		#xdata2 and ydata2 for plotting on same graph (gov speed and axle speed)
		if not xdata or not ydata: return ''		
		vinlabels = []
		vins = []

		if entryName == "VIN":
			vincount = 1
			lastelem = ydata[0]
			lastts = xdata[0]
			vinlabels.append('VIN'+str(vincount)+': ' + lastelem)
			vins.append(lastelem)
			changes = 0
			newy = ydata[:]
			for idx, elem in self.with_index(ydata):
				if elem != lastelem and elem not in vins:
					newy.insert(idx+changes,lastelem)
					xdata.insert(idx+changes,xdata[idx+changes-1])
					vincount +=1
					vinlabels.append('VIN' + str(vincount) + ': ' + elem)
					vins.append(elem)
					changes +=1
				lastelem = elem
			for label in vinlabels:
				for idx, elem in self.with_index(newy):
					if type(elem) is str and elem in label:
						newy[idx] = int(label[3:4])
			ydata = newy
			for i in range(len(vins)):
				vins[i] = vins[i].strip(' ')
		idx = 0
		xmax = max(xdata)
		ymax = max(ydata)
		xmin = min(xdata)
		ymin = min(ydata)
		ymargin = (ymax-ymin) * .3
		xmargin = (xmax-xmin) * .3
		colorstr = CHART_LINE_COLORS[idx % len(CHART_LINE_COLORS)]
		if entryName == 904:
			ytitle = 'Front Axle Speed (mph)'
		else:
			ytitle = entryName

		data = [dict(x=xdata,y=ydata,
			mode='lines+markers',
			line=dict(width=6,color='#1f3e70'),
			marker=dict(color='#adccff')
			),]

		#if gov speed exists, append its data to current data for axle speed
		if entryName == 904 and xdata2: #if axle based vehicle speed and data for gov speed is given
			#mingx = float(xdata2[0])
			#for i in range(len(xdata2)):
			#	xdata2[i] = xdata2[i] - mingx
			gdata = {
			'x': xdata2,
			'y': ydata2,
			'mode': 'lines+markers',
			'line': dict(width=6,color='#701f1f'),
			'marker': dict(color='#f28e8e')
			}
			data.append(gdata)
		
		
		layout = dict(height='80%', width='80%', plot_bgcolor='rgba(0,0,0,0)', paper_bgcolor='rgba(0,0,0,0)', showlegend=False,
			font=dict(family='"Helvetica Neue",Helvetica,Arial,sans-serif', color='#c8c8c8'),
			xaxis=dict(title='Time (s)', range=[min(xdata), max(xdata) + xmargin], autorange=False, color='#bbb'),
			yaxis=dict(title=ytitle, range=[min(ydata), max(ydata) + ymargin], autorange=False, color='#bbb'),
			title=ytitle,
			titlefont=dict(family='"Helvetica Neue",Helvetica,Arial,sans-serif', color='#c8c8c8'),
			 hovermode='closest')

		if entryName == 'VIN':
			layout['yaxis'].update(dict(tickvals=list(range(1,len(vins) + 1)),ticktext=vins,autorange=True))

		if len(data) > 1: #for plotting gov speed on same plot, edit titles
			layout['yaxis'].update(dict(autorange=True))
			layout.update(dict(showlegend=True))
			data[0].update(dict(name=ytitle))
			data[1].update(dict(name='Governor Speed (mph)'))
		
		figure = dict(data=data, layout=layout)
		plot_div = plot(figure, output_type='div', show_link=False, include_plotlyjs=True, config=dict(autosizable=True))
		return plot_div

	def get_context_data(self, *args, **kwargs):
		context = super(DisplayResultsView, self).get_context_data(**kwargs)
		resolveobj = resolve(self.request.path)
		context.update(experiment=models.Experiment.objects.get(exp_pk=resolveobj.kwargs['exp_pk']), run=models.RunResult.objects.get(pk=resolveobj.kwargs['runpk']))
		plots = []
		isOldSystem = False
		filename = context['experiment'].slugify_name() + '_' + str(context['run'].pk) + '_' + context['run'].event.start.strftime("%c").replace(' ','_') + ".csv"
		for quantity in models.ObservableQuantity.objects.filter(related_run=context['run'],plottable=True):
			plots.append(quantity.gen_plot_html())
			isOldSystem = True
		if not isOldSystem:
			KPH_PER_BIT = 1./256 # how many kph per bit for axle-based vehicle speed
			KPH_TO_MPH = 0.621371 #coversion to multiply kph and change it to mph
			GOV_SPEED_BIT_RES_MPH = 0.0049 #measured bit resolution of gov speed (mph/bit)
			vinUpdates = models.ECUUpdate.objects.filter(parent_experiment__exp_pk=context['experiment'].exp_pk,update_type=1)
			vinResponse = ['10','14','62','F1','A0']
			govUpdates = models.ECUUpdate.objects.filter(parent_experiment__exp_pk=context['experiment'].exp_pk,update_type=2)
			with open(LOG_DIRECTORY+filename, 'r') as file:
				xAxle = [] #data to contain x values (timestamps)
				yAxle = [] #data to contain y values (axlebased vehicle speed)
				xVIN = [] #data to contain x value for vin (latest timestamp of three messages)
				yVIN = [] #data to store VIN
				xGov = [] #data to contain x values for governor speed (timestamp)
				yGov = [] #data to store governor speed

				vin = '' #used to concatenate VIN
				vinCount = 0 #used to decide which message fragment of VIN the response is (0, 1, or 2)

				for line in file:
					frame = line.strip("\n").split(",")
					#each plottable quantity needs an if or elif statement here to filter the file for associated data
					if frame[1] == 'can1' and frame[3] == '18FEBF0B': #if the interface the controllers communicate on (can1). if containing axle-based vehicle speed from brake controller (0x18FEBF0B)
						#to plot axle based vehicle speed
						#print(frame)
						xAxle.append(float(frame[0]))# append timestamp
						data = frame[5].split(" ")
						yAxle.append(int(data[1] + data[0],16)*KPH_PER_BIT*KPH_TO_MPH) #first two bytes reverse byte order are axle based vehicle speed
						#must be multiplied by bit resolution to convert it to kph and by conversion factor to convert to mph
					if vinUpdates.exists() and frame[1] == 'can1' and frame[3] == '18DAF100': #if there are VIN updates, the interface is the one controllers are communicating on and the ID is the expected ID to transfer VIN
						data = frame[5].split(" ")
						if data[0:5] == vinResponse:
							vin += ''.join(data[5:])
							vinCount += 1
						elif vinCount == 1 and data[0] == '21':
							vin += ''.join(data[1:])
							vinCount += 1
						elif vinCount == 2 and data[0] == '22':
							vin += ''.join(data[1:])
							#print(bytes.fromhex(vin).decode('ascii'))
							xVIN.append(float(frame[0]))
							yVIN.append(bytes.fromhex(vin).decode('ascii'))
							vin = ''
							vinCount = 0
					if govUpdates.exists() and frame[1] == 'can1' and frame[3] == '18DAF100': #if there are gov speed updates, the interface is the one controllers are communicating on and the ID is the expected ID to transfer gov speed
						data = frame[5].split(" ")
						if data[0] == '26':
							#print(frame)
							xGov.append(float(frame[0]))
							#append timestamp
							yGov.append(float(int(data[1] + data[2],16)*GOV_SPEED_BIT_RES_MPH))
							#append govSpeed
							#print(str(govSpeed))
 
				plots.append(self.gen_plot_html(xAxle, yAxle, 904, xGov, yGov)) #entryname is 904 because axle-based vehicle speed is SPN 904
				if vinUpdates.exists() and xVIN: # if there are VIN updates, and data exists
					plots.append(self.gen_plot_html(xVIN, yVIN, "VIN", None, None))

		context.update(plots=plots)
		return context
class LogDownloadView(LoginRequiredMixin, TemplateView):

	def get_context_data(self, *args, **kwargs):
		context = super(LogDownloadView, self).get_context_data(*args, **kwargs)
		resolveobj = resolve(self.request.path)
		context.update(experiment=models.Experiment.objects.get(exp_pk=int(resolveobj.kwargs['exp_pk'])),run=models.RunResult.objects.get(pk=int(resolveobj.kwargs['runpk'])))
		return context;

	def render_to_response(self, context, **response_kwargs):
		print(os.getcwd())
		#OLD SYSTEM
		#response = HttpResponse(context['run'].log, content_type='text/csv')
		#response['Content-Disposition'] = 'attachment; filename={expname}_at_{isostartstr}.csv'.format(expname=context['experiment'].slugify_name(),isostartstr=context['run'].event.start.strftime("%c").replace(' ','_'))
		filename = context['experiment'].slugify_name() + '_' + str(context['run'].pk) + '_' + context['run'].event.start.strftime("%c").replace(' ','_') + ".csv"
		data = ""
		with open(LOG_DIRECTORY+filename, 'r') as file:
			for line in file:
				data += line
		response = HttpResponse(data, content_type='text/csv')
		response['Content-Disposition'] = 'attachment; filename = {}'.format(filename)

		return response

class LogNmftaDownloadView(LoginRequiredMixin, TemplateView):
	def get_context_data(self, *args, **kwargs):
		context = super(LogNmftaDownloadView, self).get_context_data(*args, **kwargs)
		resolveobj = resolve(self.request.path)
		context.update(experiment=models.Experiment.objects.get(exp_pk=int(resolveobj.kwargs['exp_pk'])),run=models.RunResult.objects.get(pk=int(resolveobj.kwargs['runpk'])))
		return context;

	def render_to_response(self, context, **response_kwargs):

		#NMFTA format is a sequence of blocks of 512 bytes each exactly
		newMessageFormat = b'' #stores the binary string to send in response
		firstTimestamp = None
		frameCounter = 0 #determines which frame in the block is about to be added
		#each block is 512 bytes and is lead by a header and trailed by a trailer
		blockHeader = b'\x15\x00\x00\x00' 
		blockTrailer = b'\x00\x00\x00\x00'
		#each block will have 21 CAN frames and each CAN frame is 24 bytes
		#21 frames * 24 bytes = 504 bytes. 504 + header + trailer = 512 byte block
		isFirstMessage = True #indicates if the the message is the first in the file

		CAN_ERR_FLAG = 0x20000000
		CAN_RTR_FLAG = 0x40000000

		filename = context['experiment'].slugify_name() + '_' + str(context['run'].pk) + '_' + context['run'].event.start.strftime("%c").replace(' ', '_') + ".csv"
		with open(LOG_DIRECTORY+filename, 'r') as file:
			for line in file:
				frame = line.split(",")
				print(frame)
				if frame[1] == 'can1': # for interface controllers are communicating on
					if frameCounter == 0: #if start of block append the header
						newMessageFormat += blockHeader
					
					frameCounter += 1 #increment counter for each frame

					#CAN frame consists of 4 bytes of timestamp in seconds, 4 bytes of total elapsed microseconds
					#4 bytes of DLC and microseconds portion of timestamp, 4 bytes of ID, and 8 bytes of data
					#Total bytes per frame = 24
					
					timestamp = float(frame[0])
					if isFirstMessage: #for first message initialize firstTimestamp
						firstTimestamp = timestamp
						isFirstMessage = False
					
					seconds = struct.pack("<L", int(timestamp)) #4 bytes for timestamp in seconds
					newMessageFormat += seconds
					microSecondsCounter = struct.pack("<L", int(1000000*(timestamp-int(firstTimestamp)))) #4 bytes for micro counter		
					newMessageFormat += microSecondsCounter
					micros = int(1000000*(timestamp - int(timestamp)))
					dlc = int(frame[4])
					dlcAndMicros = struct.pack("<L", ((dlc<<24) ^ (0x00FFFFFF & micros))) #compresses dlc and microseconds to 4 bytes
					newMessageFormat += dlcAndMicros
					
					if frame[2] == 'ERROR': canIdValue = int(frame[3],16) | CAN_ERR_FLAG #sets the CAN error flag
					elif frame[2] == 'RTR': canIdValue = int(frame[3],16) | CAN_RTR_FLAG #sets the CAN RTR flag
					else: canIdValue = int(frame[3],16)
					
					canId = struct.pack("<L", canIdValue) #4 bytes for ID
					newMessageFormat += canId
					for byte in frame[5].split(" "): #for each byte in CAN message
						newMessageFormat += struct.pack("<B", int(byte,16))
					
					if frameCounter == 21: #after 21 CAN frames add the trailer and reset the counter
						newMessageFormat += blockTrailer
						frameCounter = 0
						print("Block Complete")



			for i in range(21 - frameCounter): #for the rest of last block if incomplete and no more messages
				for j in range(24): #append canFrame with all 0xFF for all 24 bytes
					newMessageFormat+=b'\xFF'
			newMessageFormat += blockTrailer
			print(frameCounter, "end of file")

		response = HttpResponse(newMessageFormat, content_type='application/octet-stream')
		response['Content-Disposition'] = 'attachment; filename={}.bin'.format(filename)
		return response






class LineChartAnimatedDataView(TemplateView):
	template_name = 'experimenteditor/line_chart_animated.html'
	def get_context_data(self, *args, **kwargs):
		#for now, displays random data, hook this up to experiment data later
		context = super(LineChartAnimatedDataView, self).get_context_data(**kwargs)
		xMax = 50
		N=100
		yMax = 5
		margin = 1.5
		x = np.linspace(0,xMax,N) #should be a context argument
		y = np.random.rand(N)*yMax #should be a context argument
		#display code
		data = [dict(x=x, y=y,
			mode='lines',
			line=dict(width=2,color=CHART_LINE_COLORS[0])
			),
		]
		frames = [dict(data=[dict(
			x=x[0:k+1],
			y=y[0:k+1],
			mode='lines',
			line=dict(color=CHART_LINE_COLORS[0],width=2)
			)
		]) for k in range(N)]
		layout = dict(plot_bgcolor='transparent', paper_bgcolor='transparent', showlegend=False,
			font=dict(family='"Lato","Open Sans",Helvetica,sans-serif', color='#ffffff'),
			xaxis=dict(range=[np.min(x)-margin, np.max(x)+margin], autorange=False, color='#ddd'),
			yaxis=dict(range=[np.min(y)-margin, np.max(y)+margin], autorange=False, color='#ddd'),
			title='Random Data Test 1',#should be a context argument
			titlefont=dict(family='"Lato","Open Sans",Helvetica,sans-serif', color='#ffffff'),
			 hovermode='closest',updatemenus=[
    {
        'buttons': [
            {
            	'args': [[frames[-1]], {'frame': {'duration': 0, 'redraw': False}, 'mode': 'immediate',
                'transition': {'duration': 0}}],
                'label': 'Skip to End',
                'method': 'animate'
            }
        ],
        'font': dict(family='"Lato","Open Sans",Helvetica,sans-serif', color='#ffffff'),
        'bordercolor': '#ffffff',
        'direction': 'left',
        'pad': {'l': 10},
        'showactive': False,
        'type': 'buttons',
        'x': 0.025,
        'xanchor': 'left',
        'y': 1,
        'yanchor': 'top'
    }
])
		
		figure = dict(data=data, layout=layout, frames=frames)
		plot_html = plot_html, plot_div_id, width, height = _plot_html(figure, {'show_link': False}, True, '100%', '100%', False)
		resize_script = ''
		if width == '100%' or height == '100%':
			resize_script = (
				''
				'<script type="text/javascript">'
				'window.removeEventListener("resize" );'
				'window.addEventListener("resize", function(){{'
				'Plotly.Plots.resize(document.getElementById("{id}"));}});'
				'</script>'
			).format(id=plot_div_id)
		context['html_str'] = ''.join([plot_html,resize_script])
		return context

class LiveDataView(LoginRequiredMixin, TemplateView):
	template_name='experimenteditor/live_data.html'
	def get_context_data(self, *args, **kwargs):
		context = super(LiveDataView, self).get_context_data(*args, **kwargs)
		context['experiments'] = models.Experiment.objects.filter(created_by=self.request.user).order_by('experiment_created_date')
		return context

class LivePlotsView(LoginRequiredMixin, TemplateView):
	template_name='experimenteditor/live_plots.html'
	def get_context_data(self, *args, **kwargs):
		context = super(LivePlotsView, self).get_context_data(*args, **kwargs)
		context['experiments'] = models.Experiment.objects.filter(created_by=self.request.user).order_by('experiment_created_date')
		return context

class PinSettingsView(LoginRequiredMixin, TemplateView):
	template_name='experimenteditor/pin_settings.html'
	def get_context_data(self, *args, **kwargs):
		context = super(PinSettingsView, self).get_context_data(*args, **kwargs)
		return context


from django.conf.urls import url, include
from schedule import periods
from . import views
from schedule.views import OccurrencePreview, CreateOccurrenceView, CancelOccurrenceView
import notifications.urls
urlpatterns = [
	url(r'^linecharttest/$', views.LineChartAnimatedDataView.as_view(), name='line_chart_test'),
	url(r'^myexperiments/$', views.MyExperimentsView.as_view(), name='my_experiments'),
	url(r'^experiments/(?P<exp_pk>\d+)/quantities/$', views.EditGraphableQuantitiesView.as_view(), name='edit_quantities'),
	url(r'^experiment/(?P<exp_pk>\d+)/results/$', views.RunResultListView.as_view(), name='results_list'),
	url(r'^experiments/(?P<exp_pk>\d+)/deleteresult/(?P<runpk>\d+)/$', views.RunResultDeleteView.as_view(), name='delete_run'),
	url(r'^experiments/(?P<exp_pk>\d+)/results/(?P<runpk>\d+)/$', views.DisplayResultsView.as_view(), name='display_results'),
	url(r'^myexperiments/create/$', views.CreateExperimentView.as_view(), name='create_experiment'),
	url(r'^myexperiments/duplicate/$', views.DuplicateExperimentView.as_view(), name='copy_experiment'),
	url(r'^experiments/(?P<exp_pk>\d+)/edit/$', views.EditExperimentView.as_view(), name='edit_experiment'),
	url(r'^experiments/(?P<exp_pk>\d+)/delete/$', views.ConfirmDeleteExperimentView.as_view(), name='delete_experiment'),
	url(r'^experiments/(?P<exp_pk>\d+)/createcommand/$', views.CreateCommandView.as_view(), name='create_command'), 
	url(r'^experiments/(?P<exp_pk>\d+)/editcommand/(?P<pk2>\d+)', views.UpdateCommandView.as_view(), name='edit_command'),
	url(r'^experiments/(?P<exp_pk>\d+)/deletecommand/(?P<pk2>\d+)', views.DeleteCommandView.as_view(), name='delete_command'),
	url(r'^experiments/(?P<exp_pk>\d+)/createcancommand/$', views.CreateCanCommandView.as_view(), name='create_can_command'),
	url(r'^experiments/(?P<exp_pk>\d+)/editcancommand/(?P<pk2>\d+)', views.UpdateCanCommandView.as_view(), name='edit_can_command'),
	url(r'^experiments/(?P<exp_pk>\d+)/deletecancommand/(?P<pk2>\d+)', views.DeleteCanCommandView.as_view(), name='delete_can_command'),
	url(r'^experiments/(?P<exp_pk>\d+)/createcangencommand/$', views.CreateCanGenCommandView.as_view(), name='create_cangen_command'),
	url(r'^experiments/(?P<exp_pk>\d+)/editcangencommand/(?P<pk2>\d+)', views.UpdateCanGenCommandView.as_view(), name='edit_cangen_command'),
	url(r'^experiments/(?P<exp_pk>\d+)/deletecangencommand/(?P<pk2>\d+)', views.DeleteCanGenCommandView.as_view(), name='delete_cangen_command'),
	url(r'^experiments/(?P<exp_pk>\d+)/createecuupdate/$', views.CreateECUUpdateView.as_view(), name='create_ecu_update'),
	url(r'^experiments/(?P<exp_pk>\d+)/editecuupdate/(?P<pk2>\d+)', views.UpdateECUUpdateView.as_view(), name='edit_ecu_update'),
	url(r'^experiments/(?P<exp_pk>\d+)/deletecuupdate/(?P<pk2>\d+)', views.DeleteECUUpdateView.as_view(), name='delete_ecu_update'),
	url(r'^experiments/(?P<exp_pk>\d+)/schedule/(?P<calendar_slug>[\w-]+)/$', views.ScheduleExperimentWeekView.as_view(), name='schedule_week', kwargs={'period': periods.Week}),
	url(r'^experiments/(?P<exp_pk>\d+)/schedule/(?P<calendar_slug>[\w-]+)/create/$', views.ScheduleExperimentFormView.as_view(), name='calendar_create_event'),
	url(r'^experiments/schedule/(?P<event_id>\d+)/(?P<year>\d+)/(?P<month>\d+)/(?P<day>\d+)/(?P<hour>\d+)/(?P<minute>\d+)/(?P<second>\d+)/$', OccurrencePreview.as_view(), name='occurrence_by_date'),
	url(r'^experiments/schedule/edit/(?P<event_id>\d+)/(?P<year>\d+)/(?P<month>\d+)/(?P<day>\d+)/(?P<hour>\d+)/(?P<minute>\d+)/(?P<second>\d+)/$',
        CreateOccurrenceView.as_view(),
		name='edit_occurrence_by_date'),
	url(r'^experiments/schedule/cancel/(?P<event_id>\d+)/(?P<year>\d+)/(?P<month>\d+)/(?P<day>\d+)/(?P<hour>\d+)/(?P<minute>\d+)/(?P<second>\d+)/$',
        CancelOccurrenceView.as_view(),
		name='cancel_occurrence_by_date'),
	url(r'^notifications/', include(notifications.urls, namespace='notifications')),
	url(r'^experiments/(?P<exp_pk>\d+)/results/(?P<runpk>\d+)/logdownload/$', views.LogDownloadView.as_view(), name='download_log'),
	url(r'^experiments/(?P<exp_pk>\d+)/results/(?P<runpk>\d+)/lognmftadownload/$', views.LogNmftaDownloadView.as_view(), name='download_nmfta_log'),
	url('livedata/', views.LiveDataView.as_view(), name='live_data'),
	url('liveplots/', views.LivePlotsView.as_view(), name='live_plots'),
	url('pinsettings/', views.PinSettingsView.as_view(), name='pin_settings')
]
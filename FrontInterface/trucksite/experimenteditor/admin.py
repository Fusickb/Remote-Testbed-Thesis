from django.contrib import admin 
from .models import *

admin.site.register(Experiment)
admin.site.register(OneTimeSSSCommand)
admin.site.register(ExperimentSchedulingInfo)
admin.site.register(ObservableQuantity)
admin.site.register(CANCommand)
admin.site.register(CANGenCommand)
admin.site.register(ECUUpdate)
admin.site.register(RunResult)

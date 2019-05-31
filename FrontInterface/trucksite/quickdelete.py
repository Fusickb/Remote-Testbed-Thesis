from experimenteditor import models
from background_task.models import Task
models.ExperimentSchedulingInfo.objects.all().delete()
Task.objects.all().delete()
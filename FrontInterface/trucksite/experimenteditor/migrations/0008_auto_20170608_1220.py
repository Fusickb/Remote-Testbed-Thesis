# -*- coding: utf-8 -*-
# Generated by Django 1.11.1 on 2017-06-08 17:20
from __future__ import unicode_literals

import datetime
from django.db import migrations, models
from django.utils.timezone import utc


class Migration(migrations.Migration):

    dependencies = [
        ('experimenteditor', '0007_auto_20170608_1120'),
    ]

    operations = [
        migrations.AlterField(
            model_name='experiment',
            name='experiment_start',
            field=models.DateTimeField(default=datetime.datetime(2017, 6, 8, 17, 20, 17, 96294, tzinfo=utc), unique=True, verbose_name='Experiment Start Date'),
        ),
    ]

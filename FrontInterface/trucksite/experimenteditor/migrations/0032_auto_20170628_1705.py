# -*- coding: utf-8 -*-
# Generated by Django 1.11.1 on 2017-06-28 22:05
from __future__ import unicode_literals

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('experimenteditor', '0031_auto_20170621_1633'),
    ]

    operations = [
        migrations.AlterModelOptions(
            name='onetimessscommand',
            options={'verbose_name': 'SSS Command'},
        ),
        migrations.AlterField(
            model_name='experimentschedulinginfo',
            name='email_reminder',
            field=models.CharField(blank=True, choices=[('', 'No Reminder'), ('1day', '1 Day Before'), ('1hr', '1 Hour Before'), ('30min', '30 Minutes Before'), ('15min', '15 Minutes Before'), ('10min', '10 Minutes Before')], default='', help_text='If set, we will remind you when your experiment is running so you know when you can expect your results.', max_length=5, verbose_name='Email Reminder Time'),
        ),
        migrations.AlterField(
            model_name='onetimessscommand',
            name='commandchoice',
            field=models.CharField(choices=[('TurnIgnitionOn', 'Turn Ignition On'), ('TurnIgnitionOff', 'Turn Ignition Off'), ('SetWheelBasedVehicleSpeed', 'Set Wheel-Based Vehicle Speed'), ('SetBrakeAmount', 'Set Brake Amount (%)')], max_length=256, verbose_name='Command'),
        ),
        migrations.AlterField(
            model_name='onetimessscommand',
            name='repeat_count',
            field=models.PositiveIntegerField(blank=True, null=True, verbose_name='Execution Count'),
        ),
    ]

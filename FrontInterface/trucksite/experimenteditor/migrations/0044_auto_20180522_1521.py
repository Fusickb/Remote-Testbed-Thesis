# -*- coding: utf-8 -*-
# Generated by Django 1.11.1 on 2018-05-22 20:21
from __future__ import unicode_literals

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('experimenteditor', '0043_auto_20180522_1457'),
    ]

    operations = [
        migrations.AlterField(
            model_name='ecuupdate',
            name='vin',
            field=models.CharField(blank=True, default=None, max_length=17, null=True),
        ),
    ]

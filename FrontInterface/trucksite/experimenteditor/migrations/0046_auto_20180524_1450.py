# -*- coding: utf-8 -*-
# Generated by Django 1.11.1 on 2018-05-24 19:50
from __future__ import unicode_literals

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('experimenteditor', '0045_observablequantity_name'),
    ]

    operations = [
        migrations.AlterField(
            model_name='ecuupdate',
            name='governor_speed',
            field=models.FloatField(blank=True, default=None, null=True),
        ),
    ]

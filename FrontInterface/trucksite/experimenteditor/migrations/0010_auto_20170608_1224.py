# -*- coding: utf-8 -*-
# Generated by Django 1.11.1 on 2017-06-08 17:24
from __future__ import unicode_literals

import datetime
from django.db import migrations, models
from django.utils.timezone import utc


class Migration(migrations.Migration):

    dependencies = [
        ('experimenteditor', '0009_auto_20170608_1224'),
    ]

    operations = [
        migrations.CreateModel(
            name='SPNPGNEntry',
            fields=[
                ('id', models.AutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('pgn', models.PositiveIntegerField(blank=True, null=True)),
                ('spn', models.PositiveIntegerField(blank=True, null=True)),
                ('name', models.CharField(max_length=100)),
                ('spn_length', models.PositiveSmallIntegerField(blank=True, null=True)),
                ('description', models.TextField()),
                ('pgl', models.CharField(max_length=100)),
                ('position', models.CharField(max_length=4)),
                ('transmissionrate_ms', models.CharField(max_length=150)),
                ('units', models.CharField(max_length=20)),
                ('offset', models.CharField(max_length=15)),
            ],
        ),
        migrations.AlterField(
            model_name='experiment',
            name='experiment_start',
            field=models.DateTimeField(default=datetime.datetime(2017, 6, 8, 17, 24, 34, 909661, tzinfo=utc), unique=True, verbose_name='Experiment Start Date'),
        ),
    ]

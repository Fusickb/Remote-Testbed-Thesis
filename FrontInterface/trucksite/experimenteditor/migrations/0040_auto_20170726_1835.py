# -*- coding: utf-8 -*-
# Generated by Django 1.11.1 on 2017-07-26 23:35
from __future__ import unicode_literals

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('experimenteditor', '0039_auto_20170719_1440'),
    ]

    operations = [
        migrations.AlterField(
            model_name='cancommand',
            name='interface',
            field=models.PositiveSmallIntegerField(choices=[(1, 'can1')], default=1),
        ),
        migrations.AlterField(
            model_name='cangencommand',
            name='interface',
            field=models.PositiveSmallIntegerField(choices=[(1, 'can1')], default=1),
        ),
        migrations.AlterField(
            model_name='cangencommand',
            name='message_length',
            field=models.CharField(blank=True, help_text='Valid values: 0-8, blank (random) or i, which loops the length from 0 to 8.', max_length=1, null=True),
        ),
    ]

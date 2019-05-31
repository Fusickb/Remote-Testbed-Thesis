from django.shortcuts import render,resolve_url
from django.contrib.auth.views import LoginView
from django.views.generic import RedirectView
from django.views.generic.edit import FormView
from django.urls import reverse_lazy
from django.utils.http import is_safe_url
from django.conf import settings
from . import forms
class CorrectRedirectLoginView(RedirectView):

	def get_redirect_url(self, *args, **kwargs):
		return '/experimentscheduler/' + kwargs['redirect_url']

class CorrectRedirectLogoutView(RedirectView):

	def get_redirect_url(self, *args, **kwargs):
		return kwargs['redirect_url']

class UserCreationView(FormView):

	template_name = 'registration/register.html'
	form_class = forms.UserCreateForm
	success_url = reverse_lazy('login')

	def form_valid(self, form):
		form.save()
		return super(UserCreationView, self).form_valid(form)
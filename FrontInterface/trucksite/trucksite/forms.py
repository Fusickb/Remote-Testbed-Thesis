from django import forms
from django.contrib.auth.forms import UserCreationForm
from django.contrib.auth import models as auth_models

class UserCreateForm(UserCreationForm):
	email = forms.EmailField(required=True)

	class Meta:
		model = auth_models.User
		fields = ("username", "email", "password1", "password2")

	def save(self, commit=True):
		user = super(UserCreateForm, self).save(commit=False)
		user.email = self.cleaned_data["email"]
		if commit:
			user.save()
		return user

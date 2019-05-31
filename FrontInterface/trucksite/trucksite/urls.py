"""trucksite URL Configuration

The `urlpatterns` list routes URLs to views. For more information please see:
    https://docs.djangoproject.com/en/1.11/topics/http/urls/
Examples:
Function views
    1. Add an import:  from my_app import views
    2. Add a URL to urlpatterns:  url(r'^$', views.home, name='home')
Class-based views
    1. Add an import:  from other_app.views import Home
    2. Add a URL to urlpatterns:  url(r'^$', Home.as_view(), name='home')
Including another URLconf
    1. Import the include() function: from django.conf.urls import url, include
    2. Add a URL to urlpatterns:  url(r'^blog/', include('blog.urls'))
"""
from django.conf.urls import include, url
from django.contrib import admin
from django.urls import reverse_lazy
from django.contrib.auth import views as auth_views
from django.views.generic import RedirectView
from . import views
urlpatterns = [
	url(r'^admin/', include(admin.site.urls)),
	url(r'^accounts/', include('registration.backends.default.urls')),
    url(r'^$', RedirectView.as_view(url=reverse_lazy('my_experiments')), name='idxredir'),
	url(r'^experimenteditor/', include('experimenteditor.urls')),
	url(r'^accounts/login/$', auth_views.LoginView.as_view(template_name='registration_old/login.html'), name='login'),
	url(r'^accounts/logout/$', auth_views.LogoutView.as_view(next_page=reverse_lazy('login'), template_name='registration_old/logged_out.html'), name='auth_logout'),
	url(r'^accounts/password_reset/$', auth_views.PasswordResetView.as_view(template_name='registration_old/password_reset_form.html', success_url=reverse_lazy('password_reset_done'), email_template_name='registration_old/password_reset_email.html',subject_template_name='registration_old/password_reset_subject.txt'), name='password_reset'),
	url(r'^accounts/password_reset/done/$', auth_views.PasswordResetDoneView.as_view(template_name='registration_old/password_reset_done.html'), name='password_reset_done'),
	url(r'^accounts/password_reset_confirm/(?P<uidb64>[0-9A-Za-z_\-]+)/(?P<token>[0-9A-Za-z]{1,13}-[0-9A-Za-z]{1,20})/$', auth_views.PasswordResetConfirmView.as_view(template_name='registration_old/password_reset_confirm.html'), name='password_reset_confirm'),
	url(r'^accounts/password_reset_confirm/done/$', auth_views.PasswordResetCompleteView.as_view(template_name='registration_old/password_reset_complete.html'), name='password_reset_complete')
]
	
	
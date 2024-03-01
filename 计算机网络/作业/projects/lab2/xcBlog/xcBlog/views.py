from django.shortcuts import render, redirect
from django.http import HttpResponse
def display(request):
    return render(request, 'myweb.html')

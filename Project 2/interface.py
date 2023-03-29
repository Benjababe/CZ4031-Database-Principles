# The RDBMS query optimizer will execute a query execution plan (QEP) to process each
# such SQL query during exploration. For instance, there will be two QEPs, P and P’, associated with Q and Q’,
    
# • Design and implement an algorithm that takes as input the followings:
# a. Old query Q1, its QEP P1
# b. New query Q2, its QEP P2

# It generates a user-friendly description of what has changed from P1 to P2, and why.
# Your goal is to ensure generality of the solution (i.e., it can handle a wide variety of query plans on different database instances) 
# and the user-friendly explanation should be concise without sacrificing important information related to the plan.


# Setup Guide:
# Step 1. pip install pysimplegui
# Step 2: Run this file

import PySimpleGUI as sg

sg.theme('SandyBeach') 

# Define the layout of the interface
layout = [
    [sg.Text('Query 1:', font='Helvetica 14 bold'), sg.Multiline(size=(50, 10), key='query1')],
    [sg.Text('Query 2:', font='Helvetica 14 bold'), sg.Multiline(size=(50, 10), key='query2')],
    [sg.Button('Compare', size=(20, 2), font='Helvetica 14 bold'), sg.Button('Exit', size=(20, 2), font='Helvetica 14 bold')],
    [sg.Text('QEP P1:', font='Helvetica 14 bold'), sg.Multiline(size=(50, 10), disabled=True, key='qepP1')],
    [sg.Text('QEP P2:', font='Helvetica 14 bold'), sg.Multiline(size=(50, 10), disabled=True, key='qepP2')]
]

# Create the window
window = sg.Window('Project 2: QEP', layout)

# Event loop to process events and get inputs
while True:
    event, values = window.read()

    # If the user closes the window or clicks the Exit button
    if event == sg.WINDOW_CLOSED or event == 'Exit':
        break

    # If the user clicks the Compare button
    if event == 'Compare':
        # Get the input from the textboxes
        query1 = values['query1']
        query2 = values['query2']

        # Check if the textboxes are empty or contains whitespaces
        if not query1.strip() or not query2.strip():
            sg.popup_error('Please enter queries in both boxes.', title='Error')
        else:
            try:
                # Compare the queries and display the QEPs in the displayboxes
                qepP1 = f'{values["query1"]}'
                qepP2 = f'{values["query2"]}'
                window['qepP1'].update(qepP1)
                window['qepP2'].update(qepP2)
            except Exception as e:
                sg.popup_error(f"Error: {e}", title="Error")

# Close the window
window.close()

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
import schemdraw
from schemdraw.flow import *
import cairosvg

import psycopg2
import psycopg2.extras
from dotenv import load_dotenv
import os

from annotator import build_readable_tree, get_qep_difference, generate_numbered_list

load_dotenv()

connection = psycopg2.connect(
    dbname=os.environ.get("DB_NAME"),
    user=os.environ.get("DB_USER"),
    password=os.environ.get("DB_PASSWORD"),
    host=os.environ.get("DB_HOST"),
    port=os.environ.get("DB_PORT"),
)


def get_query_execution_plan(connection, query: str):
    with connection.cursor() as cursor:
        cursor.execute(f"EXPLAIN (FORMAT JSON) {query}")
        result = cursor.fetchone()
        return result[0][0]["Plan"]


sg.theme("SandyBeach")
font = "Helvetica 14 bold"

# Define the layout of the interface
leftColumn = [
    [
        sg.Text("Query 1:", font=font),
        sg.Multiline(size=(50, 15), key="query1"),
    ],
    [
        sg.Text("Query 2:", font=font),
        sg.Multiline(size=(50, 15), key="query2"),
    ],
    [
        sg.Button("Compare", size=(20, 2), font=font),
        sg.Button("Exit", size=(20, 2), font=font),
    ],
]

centerColumn = [
    [
        sg.Text("QEP P1:", font=font),
        sg.Multiline(size=(50, 15), disabled=True, key="qepP1"),
    ],
    [
        sg.Text("QEP P2:", font=font),
        sg.Multiline(size=(50, 15), disabled=True, key="qepP2"),
    ],
]

rightColumn = [
    [
        sg.Text("Difference:", font=font),
        sg.Multiline(size=(50, 15), disabled=True, key="qepDiff"),
    ],
    [sg.Button("View as graph", size=(20, 2), font=font, key="graph")],
]

layout = [
    [
        sg.Column(leftColumn),
        sg.VerticalSeparator(),
        sg.Column(centerColumn),
        sg.VerticalSeparator(),
        sg.Column(rightColumn),
    ]
]


class tree:
    def __init__(self, data):
        self.left = None
        self.right = None
        self.data = data


def convertArr(index, arr):
    if index >= len(arr) or arr[index] is None:
        return None
    node = tree(arr[index])
    node.left = convertArr(2 * index + 1, arr)
    node.right = convertArr(2 * index + 2, arr)
    return node


def leftArrow(node):
    return Arrow().at(node.SE).theta(-45)  # .length(d.unit / 2)


def rightArrow(node):
    return Arrow().at(node.SW).theta(-135)


nodeList = {}


def preOrder(node, d, arrow, numChild, parentIndex, childIndex):
    global leftDegree, rightDegree
    if not node:
        return

    updatedParentIndex = parentIndex

    if parentIndex == 0 and childIndex == 0:
        nodeList[0] = Start().label(node.data)
        d += nodeList[0]

    if arrow == "left":
        if numChild == 2:
            # if nodeList[childIndex+1] == None or len(arr)
            d += Arrow().at(nodeList[parentIndex].SW).theta(leftDegree)
            leftDegree += 10
        else:
            d += Arrow().down()
        nodeList[childIndex] = Start(w=4).label(node.data)  # .fill("red")
        d += nodeList[childIndex]
        updatedParentIndex = childIndex

    if arrow == "right":
        d += Arrow().at(nodeList[parentIndex].SE).theta(rightDegree)
        rightDegree -= 10
        nodeList[childIndex] = Start().label(node.data)
        d += nodeList[childIndex]
        updatedParentIndex = childIndex

    numChild = 2 - (node.left, node.right).count(None)

    preOrder(
        node.left, d, "left", numChild, updatedParentIndex, updatedParentIndex * 2 + 1
    )
    preOrder(
        node.right, d, "right", numChild, updatedParentIndex, updatedParentIndex * 2 + 2
    )


def preOrderArrow(node, d, parentIndex, childIndex):
    if not node:
        return
    if parentIndex == 0 and childIndex == 0:
        updatedParentIndex = parentIndex
    else:
        if childIndex % 2 != 0:
            d += Arrow().at(nodeList[parentIndex].SW).to(nodeList[childIndex].NE)
        else:
            d += Arrow().at(nodeList[parentIndex].SE).to(nodeList[childIndex].NW)
        updatedParentIndex = childIndex

    preOrder(node.left, d, updatedParentIndex, updatedParentIndex * 2 + 1)
    preOrder(node.right, d, updatedParentIndex, updatedParentIndex * 2 + 2)


inputList = [
    "Sort",
    "Aggregate",
    "Seq Scan",
    "Filter",
    "Sort",
    "project",
    None,
    "Aggregate",
    None,
    "Filter",
    "Sort",
    "Sort Merge Join",
]
input = convertArr(0, inputList)

# Create the window
window = sg.Window("Project 2: QEP", layout)
window2Active = False

# Event loop to process events and get inputs
def startGUI():
    while True:
        event, values = window.read()

        # If the user closes the window or clicks the Exit button
        if event == sg.WINDOW_CLOSED or event == "Exit":
            break

        # If the user clicks the Compare button
        if event == "Compare":
            # Get the input from the textboxes
            query1 = values["query1"]
            query2 = values["query2"]

            # Check if the textboxes are empty or contains whitespaces
            if not query1.strip() or not query2.strip():
                sg.popup_error("Please enter queries in both boxes.", title="Error")
            else:
                try:
                    # Compare the queries and display the QEPs in the displayboxes
                    qep1 = get_query_execution_plan(connection, query1)
                    qep2 = get_query_execution_plan(connection, query2)
                    n1 = build_readable_tree(qep1)
                    n2 = build_readable_tree(qep2)
                    diff = get_qep_difference(n1, n2)

                    window["qepP1"].update(n1)
                    window["qepP2"].update(n2)
                    window["qepDiff"].update(generate_numbered_list(diff))
                except Exception as e:
                    sg.popup_error(f"Error: {e}", title="Error")

        # if event == "graph" and not window2Active:
        #     window2Active = True
        #     with schemdraw.Drawing(show=False) as d:
        #         leftDegree = -170
        #         rightDegree = -10
        #         preOrder(input, d, None, 0, 0, 0)
        #         d.save(fname="graph1.svg")
        #         cairosvg.svg2png(url="graph1.svg", write_to="graph1.png")
        #     #     graph_image = PIL.Image.open("graph1.png")
        #     #     if graph_image.size != (1000, 1000):
        #     #         graph_image = graph_image.resize((1000, 1000), PIL.Image.ANTIALIAS)

        #     # layout2 = [
        #     #     [
        #     #         sg.Image(
        #     #             background_color="white", size=(1000, 1000), filename="graph1.png"
        #     #         )
        #     #     ]
        #     # ]
        #     layout2 = [[sg.Image(background_color="white", filename="graph1.png")]]
        #     window2 = sg.Window("QEP1 Graph View", layout2, grab_anywhere=True)

        # if window2Active:
        #     event, values = window2.read(timeout=100)
        #     if event == sg.WIN_CLOSED:
        #         window2Active = False
        #         window2.close()
    # Close the window
    window.close()

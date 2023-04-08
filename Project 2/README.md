# CZ4031 Project 2

## Running project.py

A `.env` file is required for setting up connection with the database. Ensure it has the following values. Ensure DB_NAME is whatever database holds the TPC-H dataset under the `public` schema

```
DB_NAME=xxx
DB_USER=xxx
DB_PASSWORD=xxx
DB_HOST=localhost
DB_PORT=5432
```

Python 3.11 was used during development and is recommended. 3.10 should be the minimum as structural pattern matching is used.

### Installation required 
pip install psycopg2
pip install python-dotenv
pip install pysimplegui 
pip install networkx[default] 
pip install pydot 
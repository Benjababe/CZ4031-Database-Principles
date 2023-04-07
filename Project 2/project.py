import os
import psycopg2
import psycopg2.extras

import annotator

from dotenv import load_dotenv
load_dotenv()

connection = psycopg2.connect(
    dbname=os.environ.get("DB_NAME"),
    user=os.environ.get("DB_USER"),
    password=os.environ.get("DB_PASSWORD"),
    host=os.environ.get("DB_HOST"),
    port=os.environ.get("DB_PORT")
)


def get_query_execution_plan(connection, query: str):
    with connection.cursor() as cursor:
        cursor.execute(f"EXPLAIN (FORMAT JSON) {query}")
        result = cursor.fetchone()
        return result[0][0]['Plan']


def main():
    query1 = '''
        select *
from (SELECT supplier.s_nationkey,supplier.s_suppkey FROM supplier WHERE 200<s_suppkey) AS a
join (SELECT nation.n_nationkey, nation.n_regionkey FROM nation) As b
on a.s_nationkey = b.n_nationkey
    '''
    qep1 = get_query_execution_plan(connection, query1)

    query2 = '''
        select *
from (SELECT supplier.s_nationkey,supplier.s_suppkey FROM supplier  WHERE 200>s_suppkey ORDER BY supplier.s_nationkey) AS a
join (SELECT nation.n_nationkey, nation.n_regionkey FROM nation ORDER BY nation.n_nationkey) As b
on a.s_nationkey = b.n_nationkey
    '''
    qep2 = get_query_execution_plan(connection, query2)

    n1 = annotator.build_readable_tree(qep1)
    n2 = annotator.build_readable_tree(qep2)

    print("==QEP STRUCTURES==")
    print(n1)
    print()
    print(n2)

    d = annotator.get_qep_difference(n1, n2)
    print("==DIFFERENCES==")
    print(annotator.generate_numbered_list(d))

    a = False


if __name__ == "__main__":
    main()

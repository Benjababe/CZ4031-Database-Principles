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


def cmp_nodes(n1: dict, n2: dict) -> bool:
    if n1 is None or n2 is None:
        return n1 is None and n2 is None

    return n1["Node Type"] == n2["Node Type"]


def is_subtree(t1: dict, t2: dict) -> bool:
    if t2 is None:
        return True

    if t1 is None:
        return False

    if cmp_nodes(t1, t2):
        left_is_subtree = is_subtree(t1.get("Plans", [])[0] if t1.get("Plans") else None,
                                     t2.get("Plans", [])[0] if t2.get("Plans") else None)
        right_is_subtree = is_subtree(t1.get("Plans", [])[1] if len(t1.get("Plans", [])) > 1 else None,
                                      t2.get("Plans", [])[1] if len(t2.get("Plans", [])) > 1 else None)

        if left_is_subtree and right_is_subtree:
            return True

    return is_subtree(t1.get("Plans", [])[0] if t1.get("Plans") else None, t2) or is_subtree(t1.get("Plans", [])[1] if len(t1.get("Plans", [])) > 1 else None, t2)


def contains_subtree(t1: dict, t2: dict) -> bool:
    if t2 is None:
        return True

    if t1 is None:
        return False

    if is_subtree(t1, t2):
        return True

    return contains_subtree(t1.get("Plans", [])[0] if t1.get("Plan") else None, t2) or \
        contains_subtree(t1.get("Plans", [])[1] if len(
            t1.get("Plan", [])) > 1 else None, t2)


def main():
    query1 = '''
        SELECT MAX(o_totalprice)
        FROM orders o
        JOIN lineitem li ON o.o_orderkey = li.l_orderkey
        JOIN customer c ON o.o_custkey = c.c_custkey
    '''
    qep1 = get_query_execution_plan(connection, query1)

    query2 = '''SELECT * FROM customer c, orders o WHERE c.c_custkey = o.o_custkey ORDER BY c.c_custkey'''
    qep2 = get_query_execution_plan(connection, query2)

    # x = contains_subtree(qep2, qep1)

    y = annotator.build_readable_tree(qep1)
    z = annotator.build_readable_tree(qep2)

    print(y)
    print()
    print(z)


if __name__ == "__main__":
    main()

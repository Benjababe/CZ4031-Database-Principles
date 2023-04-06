import os
import psycopg2
import psycopg2.extras

from dotenv import load_dotenv
load_dotenv()

connection = psycopg2.connect(
    dbname=os.environ.get("DB_NAME"),
    user=os.environ.get("DB_USER"),
    password=os.environ.get("DB_PASSWORD"),
    host=os.environ.get("DB_HOST"),
    port=os.environ.get("DB_PORT")
)


def get_query_execution_plan(connection, query):
    with connection.cursor() as cursor:
        cursor.execute(f"EXPLAIN (FORMAT JSON) {query}")
        result = cursor.fetchone()
        return result[0][0]['Plan']


def parse_execution_plan(plan, level=0):
    node = {
        "Node Type": plan["Node Type"],
        "Relation Name": plan.get("Relation Name"),
        "Alias": plan.get("Alias"),
        "Strategy": plan.get("Strategy"),
        "Partial Mode": plan.get("Partial Mode"),
        "Startup Cost": plan.get("Startup Cost"),
        "Total Cost": plan.get("Total Cost"),
        "Plan Rows": plan.get("Plan Rows"),
        "Plan Width": plan.get("Plan Width"),
        "Level": level,
        "Children": []
    }

    if "Plans" in plan:
        for child_plan in plan["Plans"]:
            child_node = parse_execution_plan(child_plan, level + 1)
            node["Children"].append(child_node)

    return node


def main():
    query1 = '''SELECT *
            FROM "customer", "orders"
            WHERE c_custkey = o_custkey'''
    qep1 = get_query_execution_plan(connection, query1)
    qep_tree1 = parse_execution_plan(qep1)

    query2 = '''SELECT *
            FROM "customer", "orders"
            WHERE c_custkey = o_custkey
            AND c_name LIKE \'%cheng%\''''
    qep2 = get_query_execution_plan(connection, query2)
    qep_tree2 = parse_execution_plan(qep2)


if __name__ == "__main__":
    main()

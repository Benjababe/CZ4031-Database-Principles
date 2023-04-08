import os
import psycopg2
import psycopg2.extras

from dotenv import load_dotenv
from interface import Interface

load_dotenv()

connection = psycopg2.connect(
    dbname=os.environ.get("DB_NAME"),
    user=os.environ.get("DB_USER"),
    password=os.environ.get("DB_PASSWORD"),
    host=os.environ.get("DB_HOST"),
    port=os.environ.get("DB_PORT"),
)

# def get_query_execution_plan(connection, query: str):
#     with connection.cursor() as cursor:
#         cursor.execute(f"EXPLAIN (FORMAT JSON) {query}")
#         result = cursor.fetchone()
#         return result[0][0]["Plan"]

# def main():
#     query1 = """
#         select *
# from (SELECT supplier.s_nationkey,supplier.s_suppkey FROM supplier WHERE 200<s_suppkey) AS a
# join (SELECT nation.n_nationkey, nation.n_regionkey FROM nation) As b
# on a.s_nationkey = b.n_nationkey
#     """
#     qep1 = get_query_execution_plan(connection, query1)

#     query2 = """
#         select *
# from (SELECT supplier.s_nationkey,supplier.s_suppkey FROM supplier  WHERE 200>s_suppkey ORDER BY supplier.s_nationkey) AS a
# join (SELECT nation.n_nationkey, nation.n_regionkey FROM nation ORDER BY nation.n_nationkey) As b
# on a.s_nationkey = b.n_nationkey
#     """
#     qep2 = get_query_execution_plan(connection, query2)

#     n1 = explain.build_readable_tree(qep1)
#     n2 = explain.build_readable_tree(qep2)

#     print("==QEP STRUCTURES==")
#     # print(n1)
#     # n1Nodes = createNodeList(n1)
#     # # print(n1Nodes)
#     # # print(createEdgeList(n1Nodes))
#     # # print(createNodeListDirect(n1, []))
#     # nodeList, labels, colorMap = createGraphElements(n1, [], 0)
#     # # print(nodeList)
#     # convertToGraph(n1Nodes, nodeList, "n1", 1, labels, colorMap)
#     # print(n1.name)
#     # getChild(n1, 1)
#     # for child in n1.children:
#     #     print(child.name)
#     #     if len(child.children)
#     print()
#     print(n2)
#     n2Nodes = createNodeList(n2)
#     print(n2Nodes)
#     nodeList, labels, colorMap = createGraphElements(n2, [], 0)
#     print(nodeList)
#     print(labels)
#     print(colorMap)
#     convertToGraph(n2Nodes, nodeList, "n2", 2, labels, colorMap)
#     # print(n2Nodes)
#     # print(createEdgeList(n2Nodes))
#     # convertToGraph(n2Nodes, createEdgeList(n2Nodes), "n2", 2)
#     # # print(n2.name)
#     # # getChild(n2, 1)

#     # d = annotator.get_qep_difference(n1, n2)
#     # print("==DIFFERENCES==")
#     # print(annotator.generate_numbered_list(d))

#     # a = False

if __name__ == "__main__":
    GUI = Interface(connection)
    GUI.start()
    GUI.clean()
    # main()


# example queries from https://howardlee.cn/lantern/

"""
select 
      l_returnflag,
      l_linestatus,
      sum(l_quantity) as sum_qty,
      sum(l_extendedprice) as sum_base_price,
      sum(l_extendedprice * (1 - l_discount)) as sum_disc_price,
      sum(l_extendedprice * (1 - l_discount) * (1 + l_tax)) as sum_charge,
      avg(l_quantity) as avg_qty,
      avg(l_extendedprice) as avg_price,
      avg(l_discount) as avg_disc,
      count(*) as count_order
    from
      lineitem
    where
      l_extendedprice > 100
    group by
      l_returnflag,
      l_linestatus
    order by
      l_returnflag,
      l_linestatus;
______________________________________________________________________________________

select
      l_orderkey,
      sum(l_extendedprice * (1 - l_discount)) as revenue,
      o_orderdate,
      o_shippriority
    from
      customer,
      orders,
      lineitem
    where
      c_mktsegment = 'BUILDING'
      and c_custkey = o_custkey
      and l_orderkey = o_orderkey
      and o_totalprice > 10
      and l_extendedprice > 10
    group by
      l_orderkey,
      o_orderdate,
      o_shippriority
    order by
      revenue desc,
      o_orderdate;
______________________________________________________________________________________

select
      o_orderpriority,
      count(*) as order_count
    from
      orders
    where
      o_totalprice > 100
      and exists (
        select
          *
        from
          lineitem
        where
          l_orderkey = o_orderkey
          and l_extendedprice > 100
      )
    group by
      o_orderpriority
    order by
      o_orderpriority;
______________________________________________________________________________________

select
      n_name,
      sum(l_extendedprice * (1 - l_discount)) as revenue
    from
      customer,
      orders,
      lineitem,
      supplier,
      nation,
      region
    where
      c_custkey = o_custkey
      and l_orderkey = o_orderkey
      and l_suppkey = s_suppkey
      and c_nationkey = s_nationkey
      and s_nationkey = n_nationkey
      and n_regionkey = r_regionkey
      and r_name = 'ASIA'
      and o_orderdate >= '1994-01-01'
      and o_orderdate < '1995-01-01'
      and c_acctbal > 10
      and s_acctbal > 20
    group by
      n_name
    order by
      revenue desc;
______________________________________________________________________________________

select
      supp_nation,
      cust_nation,
      l_year,
      sum(volume) as revenue
    from
      (
        select
          n1.n_name as supp_nation,
          n2.n_name as cust_nation,
          DATE_PART('YEAR',l_shipdate) as l_year,
          l_extendedprice * (1 - l_discount) as volume
        from
          supplier,
          lineitem,
          orders,
          customer,
          nation n1,
          nation n2
        where
          s_suppkey = l_suppkey
          and o_orderkey = l_orderkey
          and c_custkey = o_custkey
          and s_nationkey = n1.n_nationkey
          and c_nationkey = n2.n_nationkey
          and (
            (n1.n_name = 'FRANCE' and n2.n_name = 'GERMANY')
            or (n1.n_name = 'GERMANY' and n2.n_name = 'FRANCE')
          )
          and l_shipdate between '1995-01-01' and '1996-12-31'
          and o_totalprice > 100
          and c_acctbal > 10
      ) as shipping
    group by
      supp_nation,
      cust_nation,
      l_year
    order by
      supp_nation,
      cust_nation,
      l_year;
"""

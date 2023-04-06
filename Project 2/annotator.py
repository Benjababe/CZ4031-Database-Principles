from __future__ import annotations


intermediate_count = 0


class ReadableNode:
    type: str
    name: str
    description: str
    children: list[ReadableNode]
    table_name: str

    def __str__(self):
        return "\n".join(self.get_query_steps())

    def __init__(self, node: dict, is_root=False):
        node_type = node["Node Type"]

        self.children = []

        # generates recursively, if node has children, creates and append them to current node
        if "Plans" in node:
            for child_node in node["Plans"]:
                readable_child_node = ReadableNode(child_node)
                self.children.append(readable_child_node)

        if node_type in ["Seq Scan", "Index Scan", "Index Only Scan", "CTE Scan"]:
            self.type = "Scan"
            self.name = node_type
            self.scan_handler(node, node_type)

        elif node_type in ["Nested Loop", "Hash Join", "Merge Join"]:
            self.type = "Join"
            self.name = node_type
            self.join_handler(node, node_type, is_root)

        else:
            self.misc_handler(node, node_type, is_root)

    def scan_handler(self, node: dict, node_type: str):
        """Handles scan operations

        Args:
            node (dict): Current node in the QEP tree
            node_type (str): Type of the current node
        """

        match node_type:
            case "Seq Scan":
                rel_name = node['Relation Name']
                alias = node['Alias']

                self.table_name = rel_name
                self.description = f"Sequential scan done on {rel_name} with alias {alias}"

                if "Filter" in node:
                    self.description += f" using filter {node['Filter']}"

            case "Index Scan":
                rel_name = node['Relation Name']
                alias = node['Alias']

                self.table_name = rel_name
                self.description = f"Index scan done on {rel_name} with alias {alias}"

                if "Index Cond" in node:
                    self.description += f" with index condition {node['Index Cond']}"

                if "Filter" in node:
                    self.description += f" using filter {node['Filter']}"

            case "Index Only Scan":
                rel_name = node['Relation Name']
                alias = node['Alias']

                self.table_name = rel_name
                self.description = f"Index only scan done on {rel_name} with alias {alias}"

                if "Index Name" in node:
                    self.description += f" on index name {node['Index Name']}"

            case "CTE Scan":
                cte_name = node['CTE Name']
                alias = node['Alias']

                self.table_name = cte_name
                self.description = f"CTE scan done on {cte_name} with alias {alias}"

                if "Filter" in node:
                    self.description += f" using filter {node['Filter']}"

    def join_handler(self, node: dict, node_type: str, is_root: bool):
        """Handles JOIN operations

        Args:
            node (dict): Current node in the QEP tree
            node_type (str): Type of the current node
            is_root (bool): Flag whether the current node is the root
        """

        match node_type:
            case "Nested Loop":
                c1, c2 = self.children[:2]
                self.description = f"Nested loop join done on {c1.table_name} and {c2.table_name}"

                if not is_root:
                    self.generate_intermediate_table()

            case "Hash Join":
                c1, c2 = self.children[:2]
                self.description = f"Hash join done on {c1.table_name} and {c2.table_name}"

                if not is_root:
                    self.generate_intermediate_table()

            case "Merge Join":
                c1, c2 = self.children[:2]
                self.description = f"Merge join done on {c1.table_name} and {c2.table_name}"

                if "Merge Cond" in node:
                    self.description += f" with merge condition {node['Merge Cond']}"

                if "Filter" in node:
                    self.description += f" using filter {node['Filter']}"

                if not is_root:
                    self.generate_intermediate_table()

    def misc_handler(self, node: dict, node_type: str, is_root: bool):
        """Handles operations which are not in a group

        Args:
            node (dict): Current node in the QEP tree
            node_type (str): Type of the current node
            is_root (bool): Flag whether the current node is the root
        """

        self.type = node_type
        self.name = node_type

        match node_type:
            case "Aggregate":
                c1 = self.children[0]
                self.description = f"Aggregated table {c1.table_name}"

                if not is_root:
                    self.generate_intermediate_table()

            case "Gather":
                c1 = self.children[0]
                self.description = f"Gathered table {c1.table_name}"

                if not is_root:
                    self.generate_intermediate_table()

            case "Gather Merge":
                self.table_name = self.children[0].table_name
                self.description = f"Gathered and merged results from {self.table_name}"

            case "Hash":
                self.table_name = self.children[0].table_name
                self.description = f"Hashing done on {self.table_name}"

            case "Memoize":
                self.table_name = self.children[0].table_name
                self.description = f"Memoization done on {self.table_name}"

            case "Sort":
                key = node['Sort Key']
                self.table_name = self.children[0].table_name
                self.description = f"Sort done on {self.table_name} using sort key {key}"

    def generate_intermediate_table(self):
        """Creates an intermediate table name for the current node
        """

        global intermediate_count
        self.table_name = f"Tmp{intermediate_count}"
        intermediate_count += 1
        self.description += f" which creates intermediate table {self.table_name}"

    def get_query_steps(self) -> list[str]:
        """Retrieves the steps of the query in English

        Returns:
            list[str]: List of steps taken during the query
        """

        steps = []

        # finds earliest steps through DFS
        for child in self.children:
            steps.extend(child.get_query_steps())

        steps.append(self.description)
        return steps


def build_readable_tree(qep: dict) -> ReadableNode:
    global intermediate_count
    intermediate_count = 0

    readable_node = ReadableNode(qep, is_root=True)
    return readable_node

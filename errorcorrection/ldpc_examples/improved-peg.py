#/usr/bin/python3
# This python code implements PEG from https://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=7893750, regular code
# Usage: psd-peg.py cols rows lifting_size output
#   cols: # of columns of matrix
#   rows: # of rows of matrix       
#   symbol / check node distribution: modify hardcoded
#   print_option: See enum
# Example: psd-peg.py 12 6 600 0 1
# Example that pipes output to H folder: python3 ./psd-peg.py 12 6 600 0 1 > ./my_project_with_aff3ct/examples/bootstrap/matrices/H/test.qc
# Example that prints metadata only: python3 ./psd-peg.py 12 6 600 1 0
# Set m, n, L and varnode_degree. Algorithm will generate and print a base graph in .qc format.
# For now var node degrees are constant, but can be easily customized

# For efficiency better to use https://github.com/Lcrypto/classic-PEG-/tree/master/classic_PEG

import sys
import math
import queue
import random
import enum

###
# LDPC
###
if len(sys.argv) < 4:
    print("Insufficient args")
    print("Usage: improved-peg.py cols rows print_option")
    quit()

# Dimensions of base matrix
n = int(sys.argv[1])                # Columns of base matrix B
m = int(sys.argv[2])                # Rows of base matrix B
print_option = int(sys.argv[3])     # # Output params

class PRINT_OPTIONS(enum.Enum):
    META = 0
    META_AND_ALIST = 1
    ALIST_ONLY = 2

###
# Hardcoded data
### 

# Add your symbol node degree distribution here
vn_deg_dist = []
# For now regular
for j in range(n): vn_deg_dist.append(6)

# Add your check node degree distribution here
cn_deg_dist = []
# For now regular
for j in range(n): cn_deg_dist.append(6)

###
# INITIALIZATION OF RUNTIME / DERIVED VARIABLES
###

base_graph_girth = float('Inf')

# Permutation matrix and the result base matrix
p = [] # p(i,j) in the paper
for i in range(m):
    p.append([])
    for j in range(n):
        p[i].append(-1)

# Using enum class create enumerations
class VARS(enum.IntEnum):
   ID = 0
   DEGREES_LEFT = 1
   NEIGHBOURS = 2
   DEPTH = 3,
   IDX_IN_ARR = 4

"""
var_nodes: 
VARS.ID:    int, identifier / index. var_nodes starts from 1 & has a negative id (eww, ugly!) to differentiate them from check nodes.
VARS.DEGREES_LEFT:   int, degrees left to allocate.
VARS.NEIGHBOURS: array, check_nodes this var_node is connected to
IDX_IN_ARR: index in array

Temporary variables:
"Gamma": temp var for current iteration needed for calculating permutation if connected checknode is chosen.
         Gamma needs to be stored in the node as it is a value along the path (and there are many paths).
         Defined with a capital G because it's the capital Gamma.
         What does "Gamma" represent? It represents the accumulated permutated shift of the path. At depth = 0, Gamma = 0. When you traverse through an VARS.NEIGHBOURS,
         recall that the edge is a permutation shift.
         The Gamma stored in the nodes represent the Gamma of the shortest path to that node. 
         When it encounters a second edge, Gamma = (previous edge) - (second edge). In this implementation previous edge is just gamma.
         When it encounters subsequent nhbrs, Gamma = Gamma + [(previous edge) - (next edge)

"prevP": helps to facilitate gamma calculation

"depth": temp var for distance of checknode from current var_node at root
"parent": temp var to point to previous node

Simple object initialization (this was supposed to be a helper script, not a full-fledged implementation..)
var_nodes will not be sorted, so it doesn't require an id for each item
"""
var_nodes = []
for j in range(n):
  if (vn_deg_dist[j] > m):
    print("Error: vn_deg_dist cannot have values more than m")
    exit(-1)
  var_nodes.append([
    -(j+1), 
    vn_deg_dist[j], 
    [], 
    float("Inf"),
    j
  ])

"""
check_nodes: 
VARS.ID:    int, identifier / index, positive
VARS.DEGREES_LEFT:   int, degrees left to allocate.
VARS.NEIGHBOURS: array, var_nodes this check_node is connected to
IDX_IN_ARR: index in array

Temporary variables:
"Gamma": temp var for current iteration useful for calculating permutation if this checknode is chosen
"depth": temp var for distance of checknode from current var_node at root
""
"""
check_nodes = []
for i in range(m):
  check_nodes.append([
    i,
    cn_deg_dist[i],
    [],
    float("Inf"),
    i
  ])

# For every symbol node, link it to d_sj other check nodes
for j in range(n):
    # For the first edge established for s_j, choose the check node with the lowest degree and assign it a random permutation shift
    # This is O(n), but probably sufficient for this implementation
    c_i = max(check_nodes, key=lambda a: a[VARS.DEGREES_LEFT]) 
    # Add the edge to the edge lists
    var_nodes[j][VARS.NEIGHBOURS].append(c_i)
    c_i[VARS.NEIGHBOURS].append(var_nodes[j])
    c_i[VARS.DEGREES_LEFT] -= 1
    # Add the edge to the base case
    p[c_i[VARS.ID]][j] = 1

    # Prepare to build computational tree with current var node as the root
    # Contains ids of all visited nodes
    visited_nodes = set()
    frontier = queue.Queue()
    # Resetting is actually not necessary, but good to have. Some commented out because the others don't need to be reset
    for tmp in range(n):
        var_nodes[tmp][VARS.DEPTH] = float("Inf")
    for tmp in range(m):
        check_nodes[tmp][VARS.DEPTH] = float("Inf")

    # Build initial tree, I'll use BFS as we want the depth to be accurate (DFS may give you the wrong depth)
    # Note that in Python dictionaries are mutable, so you don't make a copy even if you put them into the queue
    # Put current var node as root
    var_nodes[j][VARS.DEPTH] = -1
    frontier.put(var_nodes[j])
    # Begin BFS
    while not frontier.empty():
        node = frontier.get()
        # Set as visited
        visited_nodes.add(node[VARS.ID])

        # Add their children if they have not yet been visited
        for child in node[VARS.NEIGHBOURS]:
            # Ensure it is not visited
            if child[VARS.ID] not in visited_nodes:
                # Calculate depth for child
                child[VARS.DEPTH] = node[VARS.DEPTH] + 1
                # Something additional I put since l isn't properly defined
                if (child[VARS.DEPTH] <= base_graph_girth * 2):
                    frontier.put(child)

    # at this point the current tree has been built and the data is stored in the check_nodes array.
    # Do a multi-condition sort: sort first by depth, tiebreak b degree
    # Depth is infinity if it is not reached, so those not in the neighbourhood will be chosen first. 
    # Sort in descending order of depth, but ascending order of degree (O(logn))
    check_nodes.sort(key=lambda a: (-a[VARS.DEPTH], -a[VARS.DEGREES_LEFT]))

    # Iterate until var node has nhbrs equal to the number of degrees; take note that we've already established 1 edge, so -1 to length
    for i in range(0, var_nodes[j][VARS.DEGREES_LEFT] - 1):
        if check_nodes[i][VARS.DEPTH] != float("Inf"): 
            # Update base grapth girth for metadata
            base_graph_girth = min(base_graph_girth, check_nodes[i][VARS.DEPTH] + 1)

        p[check_nodes[i][VARS.ID]][j] = 1

        # Update nhbrs
        var_nodes[j][VARS.NEIGHBOURS].append(check_nodes[i])
        check_nodes[i][VARS.NEIGHBOURS].append(var_nodes[j])
        check_nodes[i][VARS.DEGREES_LEFT] -= 1

# print meta
if (print_option != PRINT_OPTIONS.ALIST_ONLY):
    gb = base_graph_girth
    dv_max = max(vn_deg_dist)                                                           # max degree of var nodes
    dc_max = max(check_nodes, key=lambda a : a[VARS.DEGREES_LEFT])[VARS.DEGREES_LEFT]   # max degree of check nodes
    code_rate = float(n) / float(n+m)
    
    strf = "%-30s"
    print("Metadata:")
    print("---")
    print(strf%"m (cols): "                     + str(m))
    print(strf%"n (rows): "                     + str(n))
    print(strf%"Base graph girth: "             + str(gb))
    print(strf%"dv_max: "                       + str(dv_max))
    print(strf%"dc_max: "                       + str(dc_max))
    print(strf%"Code rate: "                    + str("%.4f"%code_rate))   # trunc to 4dp
    print()

# print matrix
if (print_option != PRINT_OPTIONS.META):
    # p is our parity check matrix. 
    # However if you recall, LDPC matrices are H = [H1 H2] matrices, where H2 is a identity matrix. So gotta concat that I to convert p to H.
    for i in range(m):
        for i2 in range(m):
            p[i].append(0 if i == i2 else -1)

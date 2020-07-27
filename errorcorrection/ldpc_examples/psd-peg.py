#/usr/bin/python3
# This python code implements PSD-PEG from https://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=7893750, regular code
# Usage: psd-peg.py cols rows varnode_deg lifting_size print_metadata print_matrix
# Example: psd-peg.py 12 6 3 600 0 1
# Example that pipes output to H folder: python3 ./psd-peg.py 12 6 3 600 0 1 > ./my_project_with_aff3ct/examples/bootstrap/matrices/H/test.qc
# Example that prints metadata only: python3 ./psd-peg.py 12 6 3 600 1 0
# Set m, n, L and varnode_degree. Algorithm will generate and print a base graph in .qc format.
# For now var node degrees are constant, but can be easily customized

import sys
import math
import queue
import random

#
# The following code section is not part of the LDPC algorithm implementation, but is a next prime number finder implementation. Jump ahead to line ~186.
# 
# https://codegolf.stackexchange.com/questions/10701/fastest-code-to-find-the-next-prime
# legendre symbol (a|m)
# note: returns m-1 if a is a non-residue, instead of -1
def legendre(a, m):
  return pow(a, (m-1) >> 1, m)

# strong probable prime
def is_sprp(n, b=2):
  d = n-1
  s = 0
  while d&1 == 0:
    s += 1
    d >>= 1

  x = pow(b, d, n)
  if x == 1 or x == n-1:
    return True

  for r in range(1, s):
    x = (x * x)%n
    if x == 1:
      return False
    elif x == n-1:
      return True

  return False

# lucas probable prime
# assumes D = 1 (mod 4), (D|n) = -1
def is_lucas_prp(n, D):
  P = 1
  Q = (1-D) >> 2

  # n+1 = 2**r*s where s is odd
  s = n+1
  r = 0
  while s&1 == 0:
    r += 1
    s >>= 1

  # calculate the bit reversal of (odd) s
  # e.g. 19 (10011) <=> 25 (11001)
  t = 0
  while s > 0:
    if s&1:
      t += 1
      s -= 1
    else:
      t <<= 1
      s >>= 1

  # use the same bit reversal process to calculate the sth Lucas number
  # keep track of q = Q**n as we go
  U = 0
  V = 2
  q = 1
  # mod_inv(2, n)
  inv_2 = (n+1) >> 1
  while t > 0:
    if t&1 == 1:
      # U, V of n+1
      U, V = ((U + V) * inv_2)%n, ((D*U + V) * inv_2)%n
      q = (q * Q)%n
      t -= 1
    else:
      # U, V of n*2
      U, V = (U * V)%n, (V * V - 2 * q)%n
      q = (q * q)%n
      t >>= 1

  # double s until we have the 2**r*sth Lucas number
  while r > 0:
      U, V = (U * V)%n, (V * V - 2 * q)%n
      q = (q * q)%n
      r -= 1

  # primality check
  # if n is prime, n divides the n+1st Lucas number, given the assumptions
  return U == 0

# primes less than 212
small_primes = set([
    2,  3,  5,  7, 11, 13, 17, 19, 23, 29,
   31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
   73, 79, 83, 89, 97,101,103,107,109,113,
  127,131,137,139,149,151,157,163,167,173,
  179,181,191,193,197,199,211])

# pre-calced sieve of eratosthenes for n = 2, 3, 5, 7
indices = [
    1, 11, 13, 17, 19, 23, 29, 31, 37, 41,
   43, 47, 53, 59, 61, 67, 71, 73, 79, 83,
   89, 97,101,103,107,109,113,121,127,131,
  137,139,143,149,151,157,163,167,169,173,
  179,181,187,191,193,197,199,209]

# distances between sieve values
offsets = [
  10, 2, 4, 2, 4, 6, 2, 6, 4, 2, 4, 6,
   6, 2, 6, 4, 2, 6, 4, 6, 8, 4, 2, 4,
   2, 4, 8, 6, 4, 6, 2, 4, 6, 2, 6, 6,
   4, 2, 4, 6, 2, 6, 4, 2, 4, 2,10, 2]

max_int = 2147483647

# an 'almost certain' primality check
def is_prime(n):
  if n < 212:
    return n in small_primes

  for p in small_primes:
    if n%p == 0:
      return False

  # if n is a 32-bit integer, perform full trial division
  if n <= max_int:
    i = 211
    while i*i < n:
      for o in offsets:
        i += o
        if n%i == 0:
          return False
    return True

  # Baillie-PSW
  # this is technically a probabalistic test, but there are no known pseudoprimes
  if not is_sprp(n): return False
  a = 5
  s = 2
  while legendre(a, n) != n-1:
    s = -s
    a = s-a
  return is_lucas_prp(n, a)

# next prime strictly larger than n
def next_prime(n):
  if n < 2:
    return 2
  # first odd larger than n
  n = (n + 1) | 1
  if n < 212:
    while True:
      if n in small_primes:
        return n
      n += 2

  # find our position in the sieve rotation via binary search
  x = int(n%210)
  s = 0
  e = 47
  m = 24
  while m != e:
    if indices[m] < x:
      s = m
      m = (s + e + 1) >> 1
    else:
      e = m
      m = (s + e) >> 1

  i = int(n + (indices[m] - x))
  # adjust offsets
  offs = offsets[m:]+offsets[:m]
  while True:
    for o in offs:
      if is_prime(i):
        return i
      i += o


# LDPC
if len(sys.argv) < 7:
    print("Insufficient args")
    print("Usage: psd-peg.py cols rows lifting_size varnode_deg print_metadata print_matrix")
    quit()

base_graph_girth = float('Inf')

# Dimensions of base matrix
n = int(sys.argv[1]) # Columns of base matrix B
m = int(sys.argv[2]) # Rows of base matrix B
vn_degree_param = int(sys.argv[3])

# Choose an L s.t. the smallest non 1 factor is not 2, and L is greater by some number specified at condition 11
# Simplest solution is just to give L a large prime number
# I'll just choose some random prime number for now
# condition 10
# L1 = 2
# Lemma 3.3
# Note that L = L1 x L2 ... Ls is just a convoluted way of saying "L is derived from the multiplication of a strictly increasing set of numbers"
# By extension L1 is the smallest non-1 distinct factor, and if it's 2 then you have a smaller girth
L = int(sys.argv[4])

print_metadata = int(sys.argv[5])
print_matrix = int(sys.argv[6])

# Permutation matrix and the result base matrix
p = [] # p(i,j) in the paper
for i in range(m):
    p.append([])
    for j in range(n):
        p[i].append(-1)

# Helper array to initialize degrees for each var node, for now I just hardcode initialize all to 6
vn_deg = []
for j in range(n):
    # vn_deg.append(random.randint(3, 10))
    vn_deg.append(vn_degree_param)
"""
var_nodes: 
"id":    int, identifier / index. var_nodes starts from 1 & has a negative id (eww, ugly!) to differentiate them from check nodes.
"deg":   int, degrees. Note that for var nodes, degrees are predefined by you
"nhbrs": array, check_nodes this var_node is connected to

Temporary variables:
"Gamma": temp var for current iteration needed for calculating permutation if connected checknode is chosen.
         Gamma needs to be stored in the node as it is a value along the path (and there are many paths).
         Defined with a capital G because it's the capital Gamma.
         What does "Gamma" represent? It represents the accumulated permutated shift of the path. At depth = 0, Gamma = 0. When you traverse through an "nhbrs",
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
    var_nodes.append({"id": -(j+1), "deg": vn_deg[j], "nhbrs": [], 
        "Gamma": 0, "prevP": 0, "parent": None, "depth": float("Inf")})

"""
check_nodes: 
"id":    int, identifier / index, positive
"deg":   int, degrees, set to 0 and calculated later
"nhbrs": array, var_nodes this check_node is connected to

Temporary variables:
"Gamma": temp var for current iteration useful for calculating permutation if this checknode is chosen
"depth": temp var for distance of checknode from current var_node at root
""
"""
check_nodes = []
for i in range(m):
    check_nodes.append({"id": i, "deg": 0, "nhbrs": [],    
        "Gamma": 0, "prevP": 0, "parent": None, "depth": float("Inf")})

# For every symbol node, link it to d_sj other check nodes
for j in range(n):
    # For the first edge established for s_j, choose the check node with the lowest degree and assign it a random permutation shift
    # This is O(n), but probably sufficient for this implementation
    c_i = min(check_nodes, key=lambda a: a["deg"]) 
    # Add the edge to the edge lists
    var_nodes[j]["nhbrs"].append(c_i)
    c_i["nhbrs"].append(var_nodes[j])
    c_i["deg"] += 1
    # Add the edge to the base case
    p[c_i["id"]][j] = random.randint(0, L)         # 0 to L - 1

    # after this we have established 1 edge, need to establish more nhbrs until fill up the degrees for the var_node

    # Goal: expand tree from sj until we've either covered the entire neighbourhood or we have expanded to the maximum depth.
    # This chunk below is pretty much nonsense copied from the paper.
    # See 13491 section top left for description on what it's trying to do here
    # Expand a tree from sj up to maximum depth l.
    # Find E^k_sj <- edge (ci, sj) of G where this edge is the kth edge incident
    # on sj and ci is a check node picked from the set where within max depth, it is not in the neighbourhood of sj
    # See definition 3.1. on p13491 regarding syntax on il etc
    # N^-lSj = Vc \ Nlsj (pg 13491 top left), Vc being the set of check nodes as defined in pg 13490, Nlsj being the set
    # consisting of all check nodes reached by a subgraph (or a tree) spreading from symbol node sj within depth l

    # Typo in the paper:
    # p(i0, j0) = edge(sj0, ci0) (permutation / path from root variable node to check node ci0 at depth 0)
    # p(i0, j1) = edge(ci0, sj1) (permutation / path from selected check node at depth 0 to child variable node at depth 1)
    # Note that they must be distinct
    # Algorithm description is SERIOUSLY lacking! Reasons:
    # 1. Why does the algorithm on page 13934 tell me to expand the same tree dsj - 1 times? What? Why not just reuse what we have already?
    #       i. Sounds stupid to me and I can't think of a reason for why it's done this way. In fact, I am almost _certain_ it is not done this way.
    #       ii. The following solution builds the existing tree and iteratively adds nhbrs based on the tree.
    #       iii. This works because checknodes that are chosen will either be outside of the tree or are leaves (as they are the furthest), thus they do not affect existing paths.
    #       iv. Taking it a step further, why not just take the checknodes and then sort by distance and subsequently sort by least degrees?
    #       v. Taking it a step even further, do we even need to construct the tree every time? (not in this implementation though)
    # 2. What is defined as "distance" ? What if you have multiple check nodes of the same distance?
    #       i. Gamma_i_t is not a suitable distance. Recall that p(x,y) is equivalent to a random variable X, and E(X) - E(X) = 0. 
    #       ii. Presumed to be distance in depth
    #       iii. Tie-break by selecting checknode with smallest degree (page 13491)
    # 3. Cases on 13491 does not account for case 4: All check nodes found in neighbourhood. Obviously establishing said edge will create a cycle.
    # 4. Define depth l. Is it a variable you preset, or a dependent on the tree? 
    #       i. Presumed to be dependent on the current tree.
    # 5. "Random permutation". What is a random permutation? Random from neg. inf to pos. inf?
    #       i. Yes, anyone worth their salt will know this is most likely 0 to (lifting size - 1). But seriously, this should be explicit.
    # 6. Gamma. Really easy to misunderstand. Definition does not explicitly state that there is a 1 to 1 mapping of Gamma to checknode.
    #       i. Algorithm doesn't explicitly mention this but it makes sense because Gamma_i_t
    #       ii. See pg13490 search for "distance"

    # Prepare to build computational tree with current var node as the root
    # Contains ids of all visited nodes
    visited_nodes = set()
    frontier = queue.Queue()
    # Resetting is actually not necessary, but good to have. Some commented out because the others don't need to be reset
    for tmp in range(n):
        var_nodes[tmp]["parent"] = None
        var_nodes[tmp]["Gamma"] = 0
        var_nodes[tmp]["prevP"] = 0
        var_nodes[tmp]["depth"] = float("Inf")
    for tmp in range(m):
        check_nodes[tmp]["parent"] = None
        check_nodes[tmp]["Gamma"] = 0
        check_nodes[tmp]["prevP"] = 0
        check_nodes[tmp]["depth"] = float("Inf")

    # Build initial tree, I'll use BFS as we want the depth to be accurate (DFS may give you the wrong depth)
    # Note that in Python dictionaries are mutable, so you don't make a copy even if you put them into the queue
    # Put current var node as root
    var_nodes[j]["Gamma"] = -1
    var_nodes[j]["depth"] = -1
    frontier.put(var_nodes[j])
    # Begin BFS
    while not frontier.empty():
        node = frontier.get()
        # Set as visited
        visited_nodes.add(node["id"])

        # If is not root
        if node["parent"] is not None:
            # Calculate Gamma (note that this is fundamentally the same as the paper but different computation procedure)
            if node["parent"]["Gamma"] == -1:                          # We are at depth = 0
                node["Gamma"] = 0                                      # init as 0
                node["prevP"] = p[node["id"]][node["parent"]["id"]]    # root node is always var_node, so we can short-cut this
            else:                                                      # We are at depth = 1 and beyond
                node["Gamma"] += node["parent"]["prevP"]               # It is always Gamma + (prevP) - (currentP)
                perm = 0
                if node["id"] < 0:                                     # node is a var node
                    perm = p[node["parent"]["id"]][node["id"]]         # parent is a check node
                else:                                                   
                    perm = p[node["id"]][node["parent"]["id"]]         # and vice-versa
                node["Gamma"] -= perm                                  # set to previous edge - current edge
                node["prevP"] = perm
            # Calculate depth
            node["depth"] = node["parent"]["depth"] + 1

        # Add their children if they have not yet been visited
        # Recall nhbrs contain indexes
        for child in node["nhbrs"]:
            # Ensure it is not visited
            if child["id"] not in visited_nodes:
                child["parent"] = node
                frontier.put(child)

    # at this point the current tree has been built and the data is stored in the check_nodes array.
    # Do a multi-condition sort: sort first by depth, tiebreak b degree
    # Depth is infinity if it is not reached, so those not in the neighbourhood will be chosen first. 
    # Sort in descending order of depth, but ascending order of degree (O(logn))
    check_nodes.sort(key=lambda a: (-a["depth"], a["deg"]))

    # Iterate until var node has nhbrs equal to the number of degrees; take note that we've already established 1 edge, so -1 to length
    for i in range(0, var_nodes[j]["deg"] - 1):
        # Random permutation
        perm = random.randint(0, L) # 0 to L - 1

        if check_nodes[i]["depth"] != float("Inf"): 
            # Case 3, and unmentioned case 4, where it's already in the neighbourhood, so cycle made
            # In such a case, need to make sure random permutation is valid
            # Lazy to optimize so I'll do a random permute with a while loop check
            # Make sure to add prevP for the last time; this will serve as the Pwhatever for condition 4
            check_nodes[i]["Gamma"] += check_nodes[i]["prevP"]
            while ((check_nodes[i]["Gamma"] - perm) % L) == 0:
                perm = random.randint(0, L) # 0 to L - 1

            # Update base grapth girth for metadata
            base_graph_girth = min(base_graph_girth, check_nodes[i]["depth"] + 1)

        p[check_nodes[i]["id"]][j] = perm

        # Update nhbrs
        var_nodes[j]["nhbrs"].append(check_nodes[i])
        check_nodes[i]["nhbrs"].append(var_nodes[j])
        check_nodes[i]["deg"] += 1

    # Revert check_nodes array back to original ordering (O(logn))
    check_nodes.sort(key=lambda a: a["id"])

# print meta
if (print_metadata != 0):
    gb = base_graph_girth
    dv_max = max(vn_deg)                                        # max degree of var nodes
    dc_max = max(check_nodes, key=lambda a : a["deg"])["deg"]   # max degree of check nodes
    L_lower_bound = math.ceil(                                  # Eqn 11, Theorem 3.6
        dv_max * (math.sqrt(math.pow(dv_max - 1, gb) * math.pow(dc_max - 1, gb)) - 1) /
        m * ((dv_max - 1) * (dc_max - 1) - 1)
    )
    L_prime = next_prime(L_lower_bound)
    # Not a perfect evaluation of L1 because L1...Lk should be distinct, meh
    multiplier = 2 if (L % 2 == 0) else 3               # represented as some Latin 'n' in the paper

    code_rate = float(n) / float(n+m)
    
    strf = "%-30s"
    print("Metadata:")
    print("---")
    print(strf%"m (cols): "                     + str(m))
    print(strf%"n (rows): "                     + str(n))
    print(strf%"var nodes' degree: "            + str(vn_degree_param))
    print(strf%"Base graph girth: "             + str(gb))
    print(strf%"dv_max: "                       + str(dv_max))
    print(strf%"dc_max: "                       + str(dc_max))
    print(strf%"Code rate: "                    + str("%.4f"%code_rate))   # trunc to 4dp
    print(strf%"L_lower_bound: "                + str(L_lower_bound))
    print(strf%"Next L prime: "                 + str(L_prime))            # A prime number ensures girth
    print(strf%"Min infobits: "                 + str(L_lower_bound * m))
    print(strf%"Infobits with L prime: "        + str(L_prime * m))
    print(strf%"Best lifted graph min girth: "  + str(3 * gb))
    print("---")
    print(strf%"Metadata with current L: "      + str(L))
    print("---")
    print(strf%"L strictly larger than bound: "    + ("Yes" if L > L_lower_bound else "No"))
    print(strf%"Infobits: "      + str(L * m))
    print(strf%"Multiplier: "    + str(multiplier))
    print(strf%"Lifted graph min girth: "  + str(multiplier * gb))
    print()

# print matrix
if (print_matrix != 0):
    # p is our parity check matrix. 
    # However if you recall, LDPC matrices are H = [H1 H2] matrices, where H2 is a identity matrix. So gotta concat that I to convert p to H.
    for i in range(m):
        for i2 in range(m):
            p[i].append(0 if i == i2 else -1)

    # Print p in .qc format
    print(str(m + n) + " " + str(m) + " " + str(L)) # Cols, rows, expansion factor
    print()
    for row in p:
        print(" ".join(str(x) for x in row))
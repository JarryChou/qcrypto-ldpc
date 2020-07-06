LDPC Protocol write-up {#ldpc_readme}
====

This README is located in `qcrypto/errorcorrection/`.

The purpose of this readme is to pen down thoughts, notes & implementation on the LDPC error correction algorithm.

## Theory

Definitions:
| Symbol  | Size  | Name  | Equivalent to  |
|---|---|---|---|
k	| k		    |   Number of bits of info                    |  
n	| n		    |   Number of bits of encoded data            |  
?	| (n, k)	|	Linear code of length n and dimension k   |  
u  	| (1* x k) 	|   Information vector (data)                 |  
G	| (k x n)	|	Generator matrix			              |  Any matrix whose rows form basis of codeword space
c  	| (1* x n) 	|   Codeword (sent data)			          |  c = uG
C	| -		    |   Codeword space                            |  
H	| (m x n)	|	Parity check matrix			              |  C is the kernel of 

1*: Presumed to be 1

Relationships:
* Rule: For any codeword c, Hc^T = 0
* Derived rule: Since G is the matrix whose rows form basis of codeword space C, HG^T = 0

Notes: 
1. G and H are binary matrices; that is, in theory, they are matrices with 0s and 1s.
2. A binary low-density parity-check (LDPC) code is a binary linear block code with a parity-check matrix that is a sparse matrix.
	i.e. H has a low density of 1s.

Original high-level conceptual procedure (See both LDPC dessert [2] & LDPC for dummies [1])
	Concept: Use LDPC itself to correct the entire message
1. Let Alice be the sender, Bob be the receiver.
2. Both Alice & Bob agree upon two matrices, G and H prior to the conversation, s.t. HG^T = 0.
3. Alice has a sequence of data she wants to send to Bob. (Let's call this d)
4. Alice calculates an additional sequence of parity bits (let's call this p)
5. Alice appends p to d. Note that d + p = u.
6. To obtain the encoded c to send to Bob, Alice does matrix multiplication: c = uG
7. Alice sends c to Bob through the noisy channel, who receives c in some form of corrupted format.
8. Bob knows that Hc^T = 0, so he somehow does correction (through belief propagation / iterative message-passing decoding) to correct the packet until Hc^T = 0

QKD high-level conceptual procedure (See: LDPC for dummies [1])
* Concept: Use LDPC as a checksum i.e. Alice sends c over to Bob who verifies if his c is equal to Alice's
1. Both Alice & Bob agree upon the matrices G and H prior to the conversation, s.t. HG^T = 0. These can be kept static.
2. Both Alice & Bob have a raw key after sifting.
3. Alice takes a block of her key as the information vector u1 and computes a codeword c1 = u1G.
	However in this case, c1 is used as a checksum.
4. Alice sends c1, as well as a hash of her key to Bob
5. Bob computes his own codeword c2 using a block of his own key and compares c1 and c2. 
6. Bob iteratively corrects his key s.t. c1 == c2 using the same procedure as above until he gets it or he reaches maximum # of iterations
7. Using the hash of the key sent by Alice, Bob verifies his key is likely equal to Alice's

Notes:
1. LDPC requires that a sufficiently large amount of sifted raw data is acquired beforehand or the hash can be potentially bruteforced
2. What should the matrices be s.t. they are useful as checksums? 
	a. How are we assured that Eve cannot obtain any information from c1?
		Sifted keys used must be longer than matrix
	b. How to obtain good error correction performance?
		1. Use matrices used in standards (but these matrices are no longer just 0s and 1s) that I don't really understand yet
		2. Use random sparse matrices (easiest solution can be improved though I think)
		3. Use existing sparse matrices we have access to
	c. How does the belief propagation work?
		We already have this in pseudo-code lmao but I don't really recall how it works

## Procedure

## Variables needed

* Initiator (Alice)
* Follower (Bob)

## References
| I  | Name  | URL |
|---|---|---|
1 | LDPC for dummies	| https://arxiv.org/ftp/arxiv/papers/1205/1205.4977.pdf
2 |  LDPC dissert.		| https://escholarship.org/uc/item/3862381k
3 | Efficient reconciliation protocol for discrete-variable quantum key distribution | - 
4 | Rate Compatible Protocol for Information Reconciliation: An application to QKD

5 |
6 |
7 |
8 |
9 |
10 | 

# How LDPC works (Lesson 1)
src: https://www.youtube.com/watch?v=ymn87tfwX60

LDPC: Low-Density Parity-Check Codes
- Block codes, every block code has a parity check matrix
[n, k, d] code
H = parity check matrix (n-k x n)
- Rows of H don't have to be linearly independent.
	- However still require the rows of H generate the dual code. This can be used to show that once again thiis implies that the nullspace of H is precisely the original code.
		- A (dv,dc) - regular LDPC code is one in which each row of H has dc 1s and each column of H contains dv 1s.
			- .: m*dc = n*dv
			- .: (m/n) = (dv/dc) (eqn 2)
		- H in this case now is now defined as a m x n matrix.
		- Since the rows don't have to be linearly independent, rank(H) <= m, 
			- .: dim(rank(H)) >= n - m
			- .: dim(code) >= n - m 
			- .: k >= n - m
			- .: Rate of code (k/n) >= 1 - (m/n) (eqn 1)
			- .: (k/n) >= 1 - (dv/dc) (eqn 3)

- total number of entries: Theta(n^2)
	- random parity check matrix: ~equal number of 0s and 1s
	- normal matrix: number of 1s in the order of n^2 as well
	- For LDPC codes: number of 1s of Theta(n)

- An example representation of LDPC codes (in the form of a tanner graph):
![](./readme_imgs/tanner_graph.png)
- This is just another representation of the parity check matrix, as illustrated below:
![](./readme_imgs/tanner_graph_2.png)
- Bipartite graph (nodes on either side can only be directly connected to nodes on thhe other side)
- Graph uniquely defines the code
- Nodes on *left* side are called **variable nodes** and they correspond to the **columns** of the parity check matrix (n of them)
	- The number of edges that each check node has on the left side represents the number of 1s in that column
- Nodes on *right* side are called **check nodes** and they correspond to the **rows** of the parity check matrix (m of them)
	- The number of edges that each check node has on the right side represents the number of 1s in that row
	- Each node represents a parity condition that must be satisfied (5 in example)
		- e.g. For node 'B', symbols 3,4,5,6,7,8 together must have even parity.
- Each edge represents a '1' in the parity check matrix
	- Number of 1s is the nuumber of edges
	- No. of edges = No. of left nodes * dv = No. of right nodes * dc
	- Typically for LDPC the block len. can be very large but we fix these degrees. So the number of 1s is automatically some multiple of n and thus Theta(n).
- In this instance, (dv=3, dc=6) - regular code
	- i.e. each node on left has degree dv=3, each node on right have degree dc=6
	- .: Rate of code = (k/n) >= 1 - (dv/dc) = 1 - (3/6) = 1/2
	- .: Rate of code >= 1/2 (Designed rate of the code)
		- Not exact because the rows are not linearly independent
	
## Why low density?
- Decode code by passing messages on the tanner graph edges.
- Start with messages from left to right, then bounce back and forth.
	- A round of messages passing from right to left followed by left to right is one iteration.
		- Fixed number of iterations (20 or 50 depending on accuracy required)
	- There is an initial phase where the messages go from left to right (which is not counted as an iteration)
	- Every time you pass a message you do some operation @ each of the nodes. 
		- Total number of operations proportionate to:
			- number of nodes
			- degree of nodes
			- number of iterations
				- if you fixed the no. of iterations, then the number of operations is linear to the nodes on the left
				- .: it is linear to the length of the code
				- .: you can decode the code in linear compexity
				- does it come at a price?
					- Performance is predictable and fast
					- ubuqitious error correction code (becoming very popular)
					- There are various possible message passing algorithms that are adapted on the Tanner graph for the LDPC codes, but there is one which goes by the name belief propagation, which goes by the name "belief propagation" which mimics more or less the messages that are passed in a junction tree. 

## LDPC Code Terminology (Lesson 2)
src: https://www.youtube.com/watch?v=3vof6zX20SI
- (dv,dc) regular code
- rate
- Tanner graph
- decoding complexity being linear

Computational Tree (another representation of the tanner graph, but it's not the same graph as the example above, but any tanner graph can be represnted as a computational tree):
![](./readme_imgs/comp-tree.png)
- For example starting with variable node 21, and they are connected to various other nodes 
- **Edges** are directed from a variable to check node e.g. 1 to A 
	- A **path** is a sequence of contiguous directed edges
		- length of path = # of directed edges in path
	- Given 2 nodes in he path, the 2 nodes are at distance d if they are connected by a path of length d but not by a path of length < d.
- An undirected **Neighborhood** of a given node *n* to depth *x* includes all the nodes that are reachable by a path length of *x* from *n*, as well as all the edges used in said paths. Notation is N^x_n.
	- For example, N^1_18 would contain nodes I, C and D and the corresponding edges between the nodes.
	- A directed neighborhood can only include nodes that are reachable by directed edges.
	- If a node a is in an undirected neighborhood of another node b, b is also in the undirected neighborhood of a to the same depth.

## LDPC Channel Models
BSC: Binary Symmetric Channel
- 2 inputs and 2 outputs
	- Inputs: xt = 1 and xt = -1
	- Output: yt = 1 and yt = -1
![](./readme_imgs/channel-model-bsc.png)
![](./readme_imgs/channel-model-bsc-2.png)
- Representing the channel multiplicatively
- Probability of flip is epsilon (error rate)
- Probability of it being correct is 1 - epsilon
- If this is confusing, -1 just stands for '1' in bit and 1 stands for '0' in bit.

Prof talks about additive White Gaussian noise model but it's not relevant to our discrete model

## Message passing terminology (Lesson 3)
src: https://www.youtube.com/watch?v=_45M-dO99-M&t=634s
- (Decoding) Passing messages between variable & check nodes
- **Iteration**:
	- Initial / zero iteration: Flow of info from variable to check node
	- Iteration 1: Flow from check to variable, then variable to check
	- Subsequent iterations follow the same process as iteration 1 
	- Variable nodes init message passing
		- They are the ones that get the information from the channel input
		- The variable nodes compute the first message they pass out
		- **Messages** are real numbers, or subset of real numbers
![](./readme_imgs/msg-pass-1.png)
- Theta / O: output alphabet of the channel 
- M: common alphabet employed to pass messages from var node to check nodes or vice versa
- They are both subsets of real numbers.

psi^0_v : O -> M (initial msg map)
- psi^0_v is a map from O to M
	- it is the initial message map
psi^(l)_c : m^(dc-1) -> m
- c: check node
- l: iteration number
psi^(l)_c : O x m^(dv-1) -> m

Assumptions concerning msg passing at a var/check node:
Mapping of the variable node:
- psi^(0)_v (bm) = psi^(0)_v (m) * b, b in {1, -1}, m in M
- psi^(0)_v (bm_0, bm_1, ... , bm_{dv-1}) = b * psi^(0)_v (m_0,m_1, ... , m_{dv-1})
![](./readme_imgs/msg-pass-2.png)
- Basically, the outgoing value of the variable node must have the same sign as the incoming value

Then the mapping at the check node I'm lazy to type out:
![](./readme_imgs/msg-pass-3.png)
- check node has degree dc in a dv,dc regular code
- dc - 1 incoming edges and one otgoing edge
- basically the check node will receive multiiple incoming messages, it will multiply the signs of all the values it gets and that is the sign of the outgoing value.
	- equivalent to a parity check

- An edge is considered an error if a sign disagrees with the sign of the corresponding code symbol.
![](./readme_imgs/msg-pass-4.png)
- Estimation of whether it is correct
- A message along an edge is said to be in error if its sign is not the true sign of the associated code symbol.
	- In this case you compare the message you're sending along the edge with the value stored in the node you are sending it to.
	- e.g. a message going from A to 6 is an indication of the value of 6, so if that sign differs from the true value of 6 then that message is an error.
	- The number of incorrect messages passed along the edges of the Tanner graph during each iteration is independent of the transmitted codeword.
		- Proof at 21:55, using a codeword of all 1s

## Gallager Decoding Algorithm A
![](./gallager-a-1.png)
- Specified specific variable and check node maps
- **Variable nodes**: 
	- Messages are all +1 -1
	- Iteration 0: The var nodes just forward what they get from the channel input
	- Iteration 1 and beyond: Messages are all +1 or -1, so it will send out the channel input *unless* there is overriding evidence from *all* its other inputs (e.g. all the other inputs say the different sign).
- **Check nodes**:
	- Messages are all +1 -1
	- Send product of incoming messages

### Evaluating performance of Gallager Decoding Algorithm A
- Do this carrying out something referred to nowadays as **density evolution**:
	- Estimate the # of incoming messgaes passed during each iteration iteratively
	- To do that we assume that the all 1s codeword was transmitted
	- Probabilities that a message passed is either -1 or 1 (34:51):
![](./gallager-a-2.png)	
![](./gallager-a-3.png)	
![](./gallager-a-4.png)	
![](./gallager-a-5.png)	
- Messsages are independent
![](./gallager-a-6.png)	
# Even Faster Geosocial Reachability Queries

<p align="justify">Geosocial reachability queries (RangeReach) determine whether a given vertex in a geosocial network can reach any 
spatial vertex within a query region. The state-of-the-art 3DReach method answers such queries by encoding graph reachability through 
interval labelling and indexing spatial vertices in a 3D R-tree. We present 2DReach, a simpler approach that avoids interval labelling entirely. Like 3DReach, 2DReach collapses strongly connected components (SCCs) into a DAG, but instead of computing interval labels, it directly stores a 2D R-tree per component over all reachable spatial vertices. A query then reduces to a single 2D R-tree lookup. We further propose compressed variants that reduce storage by excluding spatial sinks and sharing R-trees between components with identical reachable sets. Experiments on four real-world datasets show that 2DReach achieves faster index construction than 3DReach, with the compressed variant yielding the smallest index size among all methods. 2DReach delivers competitive or superior query performance with more stable response times across varying query parameters.</p>

Source code from the following paper:
- <p align="justify">Rick van der Heijden, Nikolay Yakovets, and Thekla Hamm, <i>Even Faster Geosocial Reachability Queries</i>, https://arxiv.org/abs/2602.05928</p> 

This is a fork from the source code of "Fast Geosocial Reachability Queries", available at https://github.com/pbour/rangereach. Their explanation is still available below. The information provided here is seen as an extension of the available description below.

## Runs Extension
This fork contains an extension on the original evaluation methods, additional description at [Runs](#Runs) 

### 2DReach
Prerequisites: [DAG](#directed-acyclic-graph)
```
$ ./run_2dreach.exec inputs/yelp/yelp queries/yelp/queries-range-degree.0.1-2.txt.qry
```

### 2DReach-Comp
Prerequisites: [DAG](#directed-acyclic-graph)
```
$ ./run_2dreach_comp.exec inputs/yelp/yelp queries/yelp/queries-range-degree.0.1-2.txt.qry
```

### 2DReach-Pointer
Prerequisites: [DAG](#directed-acyclic-graph)
```
$ ./run_2dreach_pointer.exec inputs/yelp/yelp queries/yelp/queries-range-degree.0.1-2.txt.qry
```

## Experiments Extension
Instructions on how to execute the experiments are in the [Experiments](#Experiments) section.

### Indexing costs Extension
The index size can be found in the execute queries report of every method.

The index time can be calculated for each method, as follows:
- 2DReach, the [create time for DAG](#directed-acyclic-graph), plus the 2D R-tree and pointer structure building time reported when executing a query file.
- 2DReach-Comp, the [create time for DAG](#directed-acyclic-graph), plus the 2D R-tree and compressed pointer structure building time reported when executing a query file.
- 2DReach-Pointer, the [create time for DAG](#directed-acyclic-graph), plus the 2D R-tree and pointer structure building time reported when executing a query file.

Indexing time of other methods can be found at the [Indexing costs](#Indexing-costs) section.

To evaluate the indexing time of the prerequisites execute the following script:
```
$ ./scripts/run_creates.sh
```


# Fast Geosocial Reachability Queries

<p align="justify">The proliferation of location-based services and social networks has given rise to <em>geosocial networks</em>, which model not only the social interactions between users but also their spatial activities. We study the efficient computation of a recently proposed query for geosocial networks called <em>Geosocial Reachability</em> query (RangeReach), which comes as a hybrid of the traditional spatial selection (range) query and the graph reachability problem. Intuitively, given a geosocial network <em>G</em>, a vertex <em>v</em>, and a spatial region <em>R</em>, RangeReach(<em>G,v,R</em>) determines whether <em>v</em> can reach any vertex in <em>G</em> with spatial activity inside <em>R</em>. We consider an interval-based labeling scheme proposed in the past for graph reachability to devise two novel solutions for RangeReach. The first takes a social-first approach, prioritizing the graph reachability predicate.  The second treats both predicates at the same time by transforming the problem of answering RangeReach queries into queries over a three-dimensional space that models the spatial and interval-based reachability information in the geosocial network. Our experimental analysis compares our proposed solutions against a baseline spatial-first approach powered by spatial indexing and a graph reachability technique, as well as the state-of-the-art method for RangeReach queries.</p>
<figure>
  <img src="./figures/running+query3.png" width="400" alt="RangeReach example" />
</figure>

Source code from the following publication:
- <p align="justify">Panagiotis Bouros, Theodoros Chondrogiannis and Daniel Kowalski, <i>Fast Geosocial Reachability Queries</i>, Proceedings of the 28th International Conference on Extending Database Technology (EDBT), https://doi.org/10.48786/edbt.2025.03, Barcelona, Spain, March 25-28 (2025)</p>

## Dependencies
- g++/gcc
- Boost Library 

## Compile
Compile the source code using ```make all``` or simply ```make```. 

## Setting up
- Download the archives for the input files from https://seafile.rlp.net/d/6fa3703e57234b6d9831/
- Extract the archives inside the inputs directory


## Creates
Tha archives already contain the files for the directed acyclic graph of the geosocial networks, the interval-base labeling scheme (original nad reverse) and a .bfl necesary for contructing the BFL scheme. Nevertheless, the following commands will create the above files from scratch. We use the ```yelp``` dataset to exemplify the commands.

### Directed Acyclic Graph
```
$ ./scripts/create_dag.sh inputs/yelp/yelp
```

### Special input file for the BFL scheme
```
$ ./create_bfl.exec inputs/yelp/yelp
```

### Interval-based labeling scheme
```
$ ./create_int.exec inputs/yelp/yelp
```

### Reverse interval-based labeling scheme 
```
$ ./create_dag.exec inputs/yelp/yelp reverse
```


## Runs
The following commands execute the evaluation methods over a query file. For the SpaReach and the 3DReach methods the creation of the 2D R-tree and the 3D R-tree is included, respectively. For the SpaReach-BFL, the creation of the BFL scheme is also included. We use again the ```yelp``` dataset to exemplify the commands.

### SpaReach-INT
Prerequisites: [DAG](#directed-acyclic-graph) and [interval-based labeling scheme](#interval-based-labeling-scheme)

Non-MBR variant
```
$ ./run_spareach-int.exec inputs/yelp/yelp queries/yelp/queries-range-degree.0.1-2.txt.qry
```

MBR variant
```
$ ./run_spareach-int_MBR.exec inputs/yelp/yelp queries/yelp/queries-range-degree.0.1-2.txt.qry
```

### SpaReach-BFL
Prerequisites: [DAG](#directed-acyclic-graph) and [.bfl input file](#special-input-file-for-the-bfl-scheme)
```
$ ./run_spareach-bfl.exec inputs/yelp/yelp queries/yelp/queries-range-degree.0.1-2.txt.qry
```

### SocReach
Prerequisites: [DAG](#directed-acyclic-graph) and [interval-based labeling scheme](#interval-based-labeling-scheme)
```
$ ./run_socreach.exec inputs/yelp/yelp queries/yelp/queries-range-degree.0.1-2.txt.qry
```

### 3DReach
Prerequisites: [DAG](#directed-acyclic-graph) and [interval-based labeling scheme](#interval-based-labeling-scheme)
```
$ ./run_3dreach.exec inputs/yelp/yelp queries/yelp/queries-range-degree.0.1-2.txt.qry
```

### 3DReach-Rev
Prerequisites: [DAG](#directed-acyclic-graph) and [reverse interval-based labeling scheme](#reverse-interval-based-labeling-scheme)
```
$ ./run_3dreach_rev.exec inputs/yelp/yelp queries/yelp/queries-range-degree.0.1-2.txt.qry
```



## Experiments
Instructions on how to execute the experiments included in the paper.

### Indexing costs
The index size can be found in the execute queries report of every method.

The index time can be calculated for each method, as follows:
- SpaReach-INT, the [create time for DAG](#directed-acyclic-graph), plus the [create time for the interval-based labeling](#interval-based-labeling-scheme), plus the 2D R-tree building time reported when executing a query file.  
- SpaReach-BFL, the [create time for DAG](#directed-acyclic-graph), plus the building time for the BFL scheme and the 2D R-tree building time both reported when executing a query file.
- SocReach, the [create time for DAG](#directed-acyclic-graph) plus the create time for interval-based labeling.  
- 3DReach, the [create time for DAG](#directed-acyclic-graph), plus the [create time for the interval-based labeling](#interval-based-labeling-scheme), plus the 3D R-tree building time reported when executing a query file.  
- 3DReach, the [create time for DAG](#directed-acyclic-graph), plus the [create time for the reverse interval-based labeling](#reverse-interval-based-labeling-scheme), plus the 3D R-tree building time reported when executing a query file.  
  

### Query performance
Execute the following scripts; output files are automatically created in the ```outputs``` directory.

#### Vary region extent
```
$ ./scripts/run_vary-region.sh
```

#### Vary degree 
```
$ ./scripts/run_vary-degree.sh
```

#### Vary spatial selectivity  
```
$ ./scripts/run_vary-selectivity.sh
```

## Notes
For the GeoReach method, the code can be found in the authors' repository https://github.com/DataSystemsLab/GeoGraphDB--Neo4j.

# MPI PAPI collector

The MPI PAPI collector allows a user quickly to gather PAPI counter statistics
for his program run. It uses the PMPI interface of the MPI standard to hook into
the MPI_Init and MPI_Finalize functions.


## Getting Started

### Prerequisites

* MPI library
* PAPI library
* C compiler

### Installation

The MPI PAPI collector comes as a single library. It can be build with
```shell
make
```

## Usage

To obtain PAPI statistics accross MPI processe preload the mpi-papi-colltor
library and specify the counters to be collected via teh MPI_PAPI_COLLECTOR_EVENTS
environment variable using the appropriate event names. Multiple events are
separated via comma.
```shell
 mpirun ... -x MPI_PAPI_COLLECTOR_EVENTS=PAPI_TOT_INS,PAPI_TOT_CYC -x LD_PRELOAD=mpi-papi-collector.so ./progam
```


## Legal Info and Contact
Copyright (c) 2019      HLRS, University of Stuttgart.
 This software is published under the terms of the BSD license.

Contact: Christoph Niethammer <niethammer@hlrs.de>



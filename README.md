## Simulation 1: star-topology.cc

+ Objective is to simulate a situation of multiple clients for one server, where the clients do not communicate between themselves. Implementing the connection with that restriction requires setting up point-to-point connections and unique IPs with the help of some loops. If only the helpers are left to do the job, the nodes do not behave as intended. Ping application was used to introduce a simple application on the network.



## Simulation 2: ping-v8-ddos.cc (not finished)

+ Based on the first simulation, this network tries to simulate a DDoS attack directed to the  server (plan to attack one client as well). Currently, it seems that the packet sink is still able to keep working normally during the attack.





## Simulation 3: multi-server.cc(not finished)

+ Objective is to simulate 2 or more servers, that communicate via point-to-point connections, and each of them have their clients that communicate between themselves (unlike in simulation 1). 
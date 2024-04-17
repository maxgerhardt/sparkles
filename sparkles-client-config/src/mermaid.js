---
title: Client
---
stateDiagram-v2
   classDef Main fill:yellow
   classDef Client fill: #f8a
       state "M: Start Main" as m0
    state "C: Start Client" as c0
    state "CC commander" as cc0
    state "M: Announcing" as m1
    state "M: Sending Timer" as m2
    state "m: in clapping mode" as m3
    state "M: requesting claps" as m4
    state "M: waiting for commands" as m5
    state "C: Waiting for announce" as c1
    state "C: Waiting for Timer" as c2
    state "C: waiting for claps" as c3
    state "C: sending claps" as c4
    state "C: waiting for animation" as c5
    state "CC: clapping mode" as cc1
    class m0, m1, m2, m3, m4, m5 Main
    class c0, c1, c2, c3, c4, c5 Client
    m0 --> m1
    m1 --> m2
    m2 --> m3
    m1 -->c1: Send address
    m3-->m4
    m4--> m5
       m2 --> c2: send timer repeatedly
    c0 --> c1
    c1 --> c2
    c2 --> c3: received timer, set delay
    c3 --> c4
    c4 --> c5
    c1 --> m2: send address
    c2 --> m2: receive timer message
    cc0-->cc1: enter clapping mode
    cc0-->m3: "ready to go" 


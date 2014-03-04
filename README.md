Download
--------

Introduction
------------

Stratego AI Evaluator is a manager program used to evaluate
the strength of ai bots used to play the game of Stratego,
a two player game in the capture-the-flag genre.
It was developed by Sam Moore of the University Computer Club
in Australia to judge a 2012 programming competition where
several stratego ai bots competed
[git repository](http://git.ucc.asn.au/?p=progcomp2012.git;a=summary).

The manager program provides a protocol
and human user interface including graphics for
the competing agents.  It can also be used by
a human for playing against the ai bots.

This repository includes the manager program
and the agents used in the 2012 competition.
It is duplicated here on github because it is used
for regression testing the stratego ai bot
[braathwaate/stratego](https://github.com/braathwaate/stratego).

Installation and Requirements
-----------------------------

Linux.  

To use graphics:
apt-get install libsdl1.2-dev

Agents
------

Peternlewis won the 2012 programming competition
and dates from 1997.
It does not use a search tree.
It uses its lowest pieces to relentlessly chase opponent
pieces around the board until a position
evolves where it can gain material.
At some point later in the game it sends its eights
to attack unmoved pieces.
Its failing is that it leaves its other pieces subject to
easy attack,
a bug that perhaps should easily be fixed.

celsius was the runner up in the 2012 programming competition.
It uses its pieces aggressively to attack unknown pieces.
Usually it ends up with losing its high pieces quickly.

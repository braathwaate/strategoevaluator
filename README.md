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
[source git repository](http://git.ucc.asn.au/?p=progcomp2012.git;a=summary).

The manager program defines a protocol (API)
and provides a human user interface including graphics for
the competing agents.  It can also be used by
a human for playing against the ai bots.

This repository includes the manager program
and the agents used in the 2012 competition.
It is duplicated here on github because it is used
for regression testing the stratego ai bot
[braathwaate/stratego](https://github.com/braathwaate/stratego).
It could also be used to expediate the
evaluation of alternative approaches
to Stratego Artificial Intelligence development,,
such as
Neural Networks,
Monte Carlo,
or other such algorithms.

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
It uses its low ranking pieces early in the game
to aggressively attack unknown and unmoved pieces.
Often it ends up losing these pieces quickly
but as often these forays result in substantial material gain,
especially eights, making it difficult to win
all games against this bot.

Like the other agents,
ramen relentlessly chases opponent pieces.
Unlike the other agents, it never gives up, creating an endless
loop.
Therefore this agent cannot be used for testing without
modifying its code to give up on endless chases.

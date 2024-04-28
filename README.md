This package contains an anagram finder and other programs for constructing words and phrases from a restricted pool of letters.

When I was growing up, a local restaurant had this puzzle on their kids' menu:

*How many words can you spell using the letters in "leprechaun"?*

One day my siblings and I took up the challenge. The 30 or so spaces they left for answers proved wildly inadequate; we had passed 100 by the time our meal was served. I think they changed the menu shortly after.

All these years later, I got curious how many more words could be spelled with those letters. Thankfully it becomes a much easier question when you have access to a fast computer and a C compiler:

`./spellable leprechaun`

And *that* is how you cheat at a game intended to distract impatient children.

The included word list [web2.txt](web2.txt) is from *Webster's Second International Dictionary* by way of [OpenBSD](https://cvsweb.openbsd.org/src/share/dict/). The 1934 copyright has lapsed.

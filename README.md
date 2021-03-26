mpml -- Multi Pattern Match Library
=====================================================
mpml is a Python module that can be used to speed up the execution of a large
collection of words. Forever, it can be also used to speed up the execution of a large
collection of regular expressions.

There is a similiar module named esmre in github. But esmre is not a  regular engine. it cannot process a
simple regexp. esmre has an excellent lib named libesm. I add some function for mpml:

save to file 
load from file
fixed match result's field: start, end  now are based unicode ,nor utf8 


For regular expressions,  i embed sregex (https://github.com/openresty/sregex).

Here is an example:

```pycon
>>> from mpml import MultiPatternMatcher, MultiRegexpMatcher
>>> index = MultiPatternMatcher()
>>> index.add_word("abc", 1)
>>> index.add_word("bc", 2)
>>> index.end()
>>> index.saveFile("1.dat")
>>> index = MultiPatternMatcher()
>>> index.loadFile("1.dat")
>>> text = "this is abc"
>>> print(index.query(text))
>>> regexp = MultiRegexpMatcher()
>>> regexp.build(["a?c", "bc"])
>>> regexp.saveFile("2.dat")
>>> regexp.loadFile("2.dat")
>>> print(regexp.query(text))
>>> 
```



# coding=utf-8
from mpml import MultiPatternMatcher, MultiRegexpMatcher

index = MultiPatternMatcher()
index.add_word("赵又廷", 1)
index.add_word("喜欢", 2)
index.end()

text = "我喜欢赵又廷"
r = index.query(text)
print(r)

regexp = MultiRegexpMatcher()
regexp.build(["我的精神家园", "精神家园","我的精神"])

text = "我的喜好包括我的精神家园"
r = regexp.query(text)
print(r)


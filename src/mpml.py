from ahcp import Index
from sregex import RegExp

class MultiPatternMatcher:
    def __init__(
        self, case_insensive=False):
        self._index = Index()
        self._case_insensive = case_insensive


    def add_word(self, word, id):
        if self._case_insensive :
            word = word.lower()
        self._index.enter(word, id)

    def end(self):
        self._index.fix()

    def saveFile(file):
        self._index.saveFile(file)

    def loadFile(self, file):
        self._index.loadFile(file)

    #############################################
    # return a list for
    # (start end, id, word)
    def query(self,text):
        q = text.lower() if self._case_insensive else text
        r = self._index.query(q)
        return [(item[0], item[1], item[2], text[item[0]:item[1]]) for item in r]

class MultiRegexpMatcher:
    def __init__(
        self):
        self._regexp = RegExp()

    def build(self, patterns):
        self._regexp.compile_multi(patterns)

    def saveFile(self, file):
        self._regexp.saveFile(file)

    def loadFile(self, file):
        self._regexp.loadFile(file)

    #############################################
    # return a list for
    # (start end, id, word)
    def query(self, text, case_insensive=False):
        q = text.lower() if case_insensive else text
        r = self._regexp.query(q)
        return [(item[0], item[1], item[2], text[item[0]:item[1]]) for item in r]

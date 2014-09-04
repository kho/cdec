#!/usr/bin/env python
import sys
import model
import sym
import log
import math

class ContextModel(model.Model):
  '''A ContextModel is one that is computed using information
  from the Context object'''

  def __init__(self, context_manager, default=0.0):
    model.Model.__init__(self)
    self.wordless = 0
    self.initial = None
    self.default = default
    self.context_manager = context_manager
    self.id = self.context_manager.add_model(self)

    '''The next feature is true if the model depends in
    some way on the entire input sentence; that is, if
    it cannot be scored when created, but must be scored
    no earlier than during the input method (note that
    this is less strict than stateful)'''
    self.contextual = True
    ''' It may seem somewhat counterintuitive that a
    ContextModel can be non-contextual, but a good
    example is the rule probabilites; although these
    are computed using the Context object, they don't
    really depend in any way on context'''


  '''inherited from model.Model, called once for each input sentence'''
  def input(self, fwords, meta, fsc=None):
    # all ContextModels must make this call
    self.context_manager.input(self, fwords, meta, fsc)


  '''This function will be called via the input method
  only for contextual models'''
  def compute_contextual_score(self, r):
    return 0.0

  '''This function is only called on rule creation for
  contextless models'''
  def compute_contextless_score(self, fphrase, ephrase, paircount, fcount, fsample_count, word_alignments):
    return 0.0

  '''Stateless models should not need to
  override this function, unless they define
  something for model.TO_GOAL'''
  def transition (self,  r, antstates, i, j, j1=None):
    return (None, 0.0)

  def estimate(self, r):
    return r.getscore("context", self.id)

  def transition(self, r, antstates, i, j, j1=None):
    return (None, r.getscore("context", self.id))

  def finaltransition(self, state):
    return 0.0

  def rescore(self, ewords, score):
    return score



'''p(e|f)'''
class EgivenF(ContextModel):

  def __init__(self, context_manager, default=0.0):
    ContextModel.__init__(self, context_manager)
    self.contextual = False

  def compute_contextless_score(self, fphrase, ephrase, paircount, fcount, fsample_count, word_alignments):
    prob = float(paircount)/float(fcount)
    return -math.log10(prob)

class CountEF(ContextModel):

        def __init__(self, context_manager, default=0.0):
                ContextModel.__init__(self, context_manager)
                self.contextual = False

        def compute_contextless_score(self, fphrase, ephrase, paircount, fcount, fsample_count, word_alignments):
                return math.log10(1.0 + float(paircount))

class CountF(ContextModel):

        def __init__(self, context_manager, default=0.0):
                ContextModel.__init__(self, context_manager)
                self.contextual = False

        def compute_contextless_score(self, fphrase, ephrase, paircount, fcount, fsample_count, word_alignments):
                return -math.log10(float(fcount))

class SampleCountF(ContextModel):

        def __init__(self, context_manager, default=0.0):
                ContextModel.__init__(self, context_manager)
                self.contextual = False

        def compute_contextless_score(self, fphrase, ephrase, paircount, fcount, fsample_count, word_alignments):
                return math.log10(1.0 + float(fsample_count))



class EgivenFCoherent(ContextModel):

  def __init__(self, context_manager, default=0.0):
    ContextModel.__init__(self, context_manager)
    self.contextual = False

  def compute_contextless_score(self, fphrase, ephrase, paircount, fcount, fsample_count, word_alignments):
    prob = float(paircount)/float(fsample_count)
    #print "paircount=",paircount," , fsample_count=",fsample_count,", prob=",prob
    if (prob == 0.0): return 99.0
    return -math.log10(prob)



class CoherenceProb(ContextModel):

  def __init__(self, context_manager, default=0.0):
    ContextModel.__init__(self, context_manager)
    self.contextual = False

  def compute_contextless_score(self, fphrase, ephrase, paircount, fcount, fsample_count, word_alignments):
    prob = float(fcount)/float(fsample_count)
    return -math.log10(prob)

class LexEgivenF(ContextModel):

  def __init__(self, context_manager, ttable, col=0):
    ContextModel.__init__(self, context_manager)
    self.ttable = ttable
    self.col = col
    self.wordless = 0
    self.initial = None
    self.contextual = False


  def compute_contextless_score(self, fphrase, ephrase, paircount, fcount, fsample_count, word_alignments):
    #print str(fphrase), " ||| ", str(ephrase)
    #alignstr = []
    #for i in xrange(len(word_alignments)):
    #  alignstr.append("%d-%d" % (word_alignments[i]/65536, word_alignments[i]%65536))
    #print " ".join(alignstr)
    #print "LexEgivenF"
    
    fwords = map(sym.tostring, fphrase)
    ewords = map(sym.tostring, ephrase)
    
    e_prob = []
    e_align_count = []
    e_is_val = []
    for  i in xrange(len(ephrase)):
      e_prob.append(0)
      e_align_count.append(0)
      if sym.isvar(ephrase[i]):
        e_is_val.append(1)
      else:
        e_is_val.append(0)
          
    for  i in xrange(len(word_alignments)):
      f_index = word_alignments[i]/65536
      e_index = word_alignments[i]%65536
      if (e_is_val[e_index] == 1):
        raise Exception("ERROR1")
      
      score = self.ttable.get_score(fwords[f_index], ewords[e_index], self.col)
      if (score == None):
        raise Exception("ERROR2")
      #print "score(", fwords[f_index], ", ", ewords[e_index], ", ", self.col, ")=", score
      e_prob[e_index] += score
      e_align_count[e_index] += 1
    
    totalscore = 0.0  
    for  i in xrange(len(ephrase)):
      if (e_is_val[i] == 0):
        if (e_align_count[i] == 0):
          score = self.ttable.get_score("NULL", ewords[i], self.col)
          if (score == None):
            raise Exception("ERROR2")
          #print "score(", "NULL", ", ", ewords[i], ", ", self.col, ")=", score
          e_prob[i] += score
          e_align_count[i] += 1
        #print "totalscore* =", e_prob[i] / e_align_count[i]
        totalscore -= math.log10(e_prob[i] / e_align_count[i])
        
    return totalscore
    #if totalscore == 0.0:
    #  return 999
    #else:
    #  return -math.log10(totalscore)

class MaxLexEgivenF(ContextModel):

  def __init__(self, context_manager, ttable, col=0):
    ContextModel.__init__(self, context_manager)
    self.ttable = ttable
    self.col = col
    self.wordless = 0
    self.initial = None
    self.contextual = False


  def compute_contextless_score(self, fphrase, ephrase, paircount, fcount, fsample_count, word_alignments):
    totalscore = 1.0
    fwords = map(sym.tostring, filter(lambda x: not sym.isvar(x), fphrase))
    fwords.append("NULL")
    ewords = map(sym.tostring, filter(lambda x: not sym.isvar(x), ephrase))
    #print "MaxLexEgivenF"
    for e in ewords:
      maxScore = 0.0
      for f in fwords:
        score = self.ttable.get_score(f, e, self.col)
        #print "score(", f, ", ", e, ", ", self.col, ")=", score
        #print "score(MaxLexEgivenF) = ",score
        if score > maxScore:
          maxScore = score
      totalscore *= maxScore
    if totalscore == 0.0:
      return 999
    else:
      return -math.log10(totalscore)


class LexFgivenE(ContextModel):

  def __init__(self, context_manager, ttable, col=1):
    ContextModel.__init__(self, context_manager)
    self.ttable = ttable
    self.col = col
    self.wordless = 0
    self.initial = None
    self.contextual = False


  def compute_contextless_score(self, fphrase, ephrase, paircount, fcount, fsample_count, word_alignments):
    #print ""
    #print str(fphrase), " ||| ", str(ephrase)
    #alignstr = []
    #for  i in xrange(len(word_alignments)):
    #  alignstr.append("%d-%d" % (word_alignments[i]/65536, word_alignments[i]%65536))
    #print " ".join(alignstr)
    #print "LexFgivenE"
    
    fwords = map(sym.tostring, fphrase)
    ewords = map(sym.tostring, ephrase)
    
    f_prob = []
    f_align_count = []
    f_is_val = []
    for i in xrange(len(fphrase)):
      f_prob.append(0)
      f_align_count.append(0)
      if sym.isvar(fphrase[i]):
        f_is_val.append(1)
      else:
        f_is_val.append(0)
          
    for  i in xrange(len(word_alignments)):
      f_index = word_alignments[i]/65536
      e_index = word_alignments[i]%65536
      
      if (f_is_val[f_index] == 1):
        raise Exception("ERROR1")
      
      score = self.ttable.get_score(fwords[f_index], ewords[e_index], self.col)
      if (score == None):
        raise Exception("ERROR2")
      #print "score(", fwords[f_index], ", ", ewords[e_index], ", ", self.col, ")=", score
      f_prob[f_index] += score
      f_align_count[f_index] += 1
    
    totalscore = 0.0  
    for  i in xrange(len(fphrase)):
      if (f_is_val[i] == 0):
        if (f_align_count[i] == 0):
          score = self.ttable.get_score(fwords[i], "NULL", self.col)
          if (score == None):
            raise Exception("ERROR2")
          #print "score(", fwords[i], ", ", "NULL", ", ", self.col, ")=", score
          f_prob[i] += score
          f_align_count[i] += 1
        #print "totalscore *=", f_prob[i] / f_align_count[i]
        totalscore -= math.log10(f_prob[i] / f_align_count[i])
        
    return totalscore
    #if totalscore == 0.0:
    #  return 999
    #else:
    #  return -math.log10(totalscore)
  
class MaxLexFgivenE(ContextModel):

  def __init__(self, context_manager, ttable, col=1):
    ContextModel.__init__(self, context_manager)
    self.ttable = ttable
    self.col = col
    self.wordless = 0
    self.initial = None
    self.contextual = False


  def compute_contextless_score(self, fphrase, ephrase, paircount, fcount, fsample_count, word_alignments):
    totalscore = 1.0
    fwords = map(sym.tostring, filter(lambda x: not sym.isvar(x), fphrase))
    ewords = map(sym.tostring, filter(lambda x: not sym.isvar(x), ephrase))
    ewords.append("NULL")
    #print "MaxLexFgivenE"
    for f in fwords:
      maxScore = 0.0
      for e in ewords:
        score = self.ttable.get_score(f, e, self.col)
        #print "score(", f, ", ", e, ", ", self.col, ")=", score
        #print "score(MaxLexFgivenE) = ",score
        if score > maxScore:
          maxScore = score
      totalscore *= maxScore
    if totalscore == 0.0:
      return 999
    else:
      return -math.log10(totalscore)


class IsSingletonF(ContextModel):

        def __init__(self, context_manager, default=0.0):
                ContextModel.__init__(self, context_manager)
                self.contextual = False

        def compute_contextless_score(self, fphrase, ephrase, paircount, fcount, fsample_count, word_alignments):
                return (fcount==1)


class IsSingletonFE(ContextModel):

        def __init__(self, context_manager, default=0.0):
                ContextModel.__init__(self, context_manager)
                self.contextual = False

        def compute_contextless_score(self, fphrase, ephrase, paircount, fcount, fsample_count, word_alignments):
                return (paircount==1)

class IsNotSingletonF(ContextModel):

        def __init__(self, context_manager, default=0.0):
                ContextModel.__init__(self, context_manager)
                self.contextual = False

        def compute_contextless_score(self, fphrase, ephrase, paircount, fcount, fsample_count, word_alignments):
                return (fcount>1)


class IsNotSingletonFE(ContextModel):

        def __init__(self, context_manager, default=0.0):
                ContextModel.__init__(self, context_manager)
                self.contextual = False

        def compute_contextless_score(self, fphrase, ephrase, paircount, fcount, fsample_count, word_alignments):
                return (paircount>1)


class IsFEGreaterThanZero(ContextModel):

        def __init__(self, context_manager, default=0.0):
                ContextModel.__init__(self, context_manager)
                self.contextual = False

        def compute_contextless_score(self, fphrase, ephrase, paircount, fcount, fsample_count, word_alignments):
                return (paircount > 0.01)


class SingleF(ContextModel):

        def __init__(self, context_manager, default=0.0):
                ContextModel.__init__(self, context_manager)
                self.contextual = False

        def compute_contextless_score(self, fphrase, ephrase, paircount, fcount, fsample_count, word_alignments):
                return (fcount==1)


class SingleRule(ContextModel):

        def __init__(self, context_manager, default=0.0):
                ContextModel.__init__(self, context_manager)
                self.contextual = False

        def compute_contextless_score(self, fphrase, ephrase, paircount, fcount, fsample_count, word_alignments):
                return (paircount==1)



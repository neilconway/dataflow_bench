require 'benchmark'

class Pred
  def initialize(next_op, pred)
    @next_op = next_op
    @pred = pred
  end

  def process(t)
    @next_op.process(t) if t != @pred
  end
end

class Join
  def initialize(next_op)
    @next_op = next_op
    @tbl = {}
    12000.times do |i|
      @tbl[i] = true if i % 2 == 0
    end
  end

  def process(t)
    @next_op.process(t) unless @tbl.has_key? t
  end
end

class Sink
  def initialize
    @store = []
  end

  def process(t)
    @store << t
  end
end

def do_bench
  pipe = []
  pipe.unshift(Sink.new)
  pipe.unshift(Pred.new(pipe.first, 4))
  pipe.unshift(Pred.new(pipe.first, 6))
  pipe.unshift(Join.new(pipe.first))
  pipe.unshift(Pred.new(pipe.first, 8))
  pipe.unshift(Pred.new(pipe.first, 10))
  pipe.unshift(Join.new(pipe.first))
  p = pipe.first

  s = Benchmark.measure do
    2000000.times do |i|
      p.process(i) if i % 2 == 1
    end
  end

  puts s
end

10.times { do_bench }

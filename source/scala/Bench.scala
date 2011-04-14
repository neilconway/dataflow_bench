import scala.collection.mutable.ArrayBuffer

trait Operator {
  def process(t: Int): Unit
}

class SinkOp extends Operator {
  private val buf = new ArrayBuffer[Int]

  def process(t: Int): Unit = {
    buf += t
  }
}

class PredOp(next: Operator, pred: Int) extends Operator {
  def process(t: Int): Unit = {
    if (t != pred)
      next.process(t)
  }
}

class JoinOp(next: Operator) extends Operator {
  private val tbl = ((1 to 12000) filter {_ % 2 == 0}).toSet

  def process(t: Int): Unit = {
    if (tbl.contains(t) == false)
      next.process(t)
  }
}

object Bench {
  def main(args: Array[String]) {
    for (i <- 1 to 10) {
      val t0 = System.currentTimeMillis()
      doBench()
      val t1 = System.currentTimeMillis()
      println((t1 - t0).asInstanceOf[Float] + " msecs")
    }
  }

  def doBench():Unit = {
    val pipe = new ArrayBuffer[Operator]
    pipe.prepend(new SinkOp)
    pipe.prepend(new PredOp(pipe(0), 4))
    pipe.prepend(new PredOp(pipe(0), 6))
    pipe.prepend(new JoinOp(pipe(0)))
    pipe.prepend(new PredOp(pipe(0), 8))
    pipe.prepend(new PredOp(pipe(0), 10))
    pipe.prepend(new JoinOp(pipe(0)))

    val head = pipe(0)
    for (i <- 0 to 1999999) {
      if (i % 2 == 1)
        head.process(i)
    }
  }
}

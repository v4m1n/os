module;

module sync;
import cpu;
import debug;


void spinlock::lock() {
  while (xchg(locked_, 1)) cbarrier();
}
void spinlock::unlock() {
  locked_ = 0;
}
bool spinlock::try_lock() {
  return xchg(locked_, 1);
}

void spinlock_irq::lock() {
  auto flg = irq::getIF();
  irq::disableInterrupts();
  while (xchg(locked_, 1)) cbarrier();
  if_ = flg;
}
void spinlock_irq::unlock() {
  auto flg = if_;
  locked_ = 0;
  if (flg)
    irq::enableInterrupts();
}
bool spinlock_irq::try_lock() {
  auto flg = irq::getIF();
  irq::disableInterrupts();
  auto ret = xchg(locked_, 1);
  if (ret && flg) {
    irq::enableInterrupts();
  }
  return ret;
}

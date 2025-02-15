import abilityTest from './Ability.test';
import LruTest from './LruBufferPool.test'
export default function testsuite() {
  LruTest();
  abilityTest();
}
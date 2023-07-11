from unittest import TestCase, main as unittest_main
import openexr


class MinimalTests(TestCase):
    def setUp(self):
        pass

    def test_one_iter(self):
        print(openexr.__version__)


if __name__ == '__main__':
    unittest_main() 

(defun fib (n) (if (< n 3) 1 (+ (fib (- n 1)) (fib (- n 2)))))
(assert (= 832040 (fib 30)))

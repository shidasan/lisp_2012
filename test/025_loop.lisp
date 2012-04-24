(setq x 0)
(setq y 0)
(loop 
 (if (= x 100) (return nil)
 (progn
  (print x)
  (setq y (+ y x))
  (setq x (+ x 1)))))
(assert (= y 4950))

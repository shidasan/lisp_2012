(defun f () (setq x 1) (setq y 2))
(f)
(assert (= 2 y))

(setq myadd (defun add (x y) (+ x y)))
(assert (= (funcall myadd 1 2) 3))

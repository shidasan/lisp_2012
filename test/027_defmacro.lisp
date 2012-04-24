(defmacro inc (var) (list 'setq var (list '+ var 1)))
(setq x 1)
(inc x)
(assert (= x 2))

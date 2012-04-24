(defun itemCheck (tree)
 (if (= (length tree) 1)
  (car tree)
  (+ (car tree) (- (itemCheck (car (cdr tree))) (itemCheck (car (cdr (cdr tree))))))
 ))
(defun bottomUpTree (item depth)
 (if (= depth 0)
  (list item)
  (list item (bottomUpTree (- (* item 2) 1) (- depth 1)) (bottomUpTree (* item 2) (- depth 1)))
 ))
(defun multipile (n i)
 (if (= i 0) 1 (* n (multipile n (- i 1)))))
(defun exec(n)
 (let ((stretchDepth (+ n 1))(longLived (bottomUpTree 0 (+ n 1)))(depth 4) iteration check i)
  (format T "stretch tree of depth ~A check: ~A~%" stretchDepth (itemCheck (bottomUpTree 0 10)))
  (loop
   (if (> depth n) (return nil)
	(progn
    (setq iteration (multipile 2 (+ 4 (- n depth))))
    (setq check 0)
    (setq i 1)
    (loop
	 (if (> i iteration) (return nil)
	  (progn 
	  (setq check (+ check (itemCheck (bottomUpTree i depth))))
	  (setq check (+ check (itemCheck (bottomUpTree (- 0 i) depth))))
	  (setq i (+ i 1)))))
	(format T "~A trees of depth ~A check: ~A~%" (* iteration 2) depth check)
	(setq depth (+ depth 2)))))
  (format T "long lived tree of depth ~A check: ~A~%" n (itemCheck longLived))))
(exec 8)

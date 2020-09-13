(definstrument (zc time dur freq amp length1 length2 feedback)
  (let* ((beg (seconds->samples time))
	 (end (+ beg (seconds->samples dur)))
	 (s (make-pulse-train :frequency freq))
	 (d0 (make-comb :size length1 :max-size (+ 1 (max length1 length2)) :scaler feedback))
	 (zenv (make-env '(0 0 1 1) :scaler (- length2 length1) :duration dur)))
    (run
     (do ((i beg (+ i 1)))
	 ((= i end))
       (outa i (comb d0 (* amp (pulse-train s)) (env zenv)))))))

;;(with-sound () (zc 0 3 100 .1 20 100 .95) (zc 3.5 3 100 .1 100 20 .95))


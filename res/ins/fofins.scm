;;; -------- FOF example

(definstrument (fofins beg dur frq amp vib f0 a0 f1 a1 f2 a2 (ae '(0 0 25 1 75 1 100 0)) ve)
  "(fofins beg dur frq amp vib f0 a0 f1 a1 f2 a2 (ampenv '(0 0 25 1 75 1 100 0)) vibenv) produces FOF 
synthesis: (fofins 0 1 270 .2 .001 730 .6 1090 .3 2440 .1)"
    (let* ((two-pi (* 2 pi))
	   (start (seconds->samples beg))
	   (len (seconds->samples dur))
	   (end (+ start len))
	   (ampf (make-env ae :scaler amp :duration dur))
	   (vibf (make-env (or ve (list 0 1 100 1)) :scaler vib :duration dur))
	   (frq0 (hz->radians f0))
	   (frq1 (hz->radians f1))
	   (frq2 (hz->radians f2))
	   (foflen (if (= (mus-srate) 22050) 100 200))
	   (vibr (make-oscil 6))
	   (win-freq (/ two-pi foflen))
	   (wt0 (make-wave-train :size foflen :frequency frq))
	   (foftab (mus-data wt0)))
      (do ((i 0 (+ i 1)))
	  ((= i foflen))
	(set! (foftab i) (* (+ (* a0 (sin (* i frq0)))
			       (* a1 (sin (* i frq1)))
			       (* a2 (sin (* i frq2))))
			    .5 (- 1.0 (cos (* i win-freq))))))
      (run
       (do ((i start (+ i 1)))
	   ((= i end))
	 (outa i (* (env ampf) 
		    (wave-train wt0 (* (env vibf) 
				       (oscil vibr)))))))))


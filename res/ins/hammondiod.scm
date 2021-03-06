(definstrument (hammondoid beg dur freq amp)
  ;; from Perry Cook's BeeThree.cpp
  (let* ((osc0 (make-oscil (* freq 0.999)))
	 (osc1 (make-oscil (* freq 1.997)))
	 (osc2 (make-oscil (* freq 3.006)))
	 (osc3 (make-oscil (* freq 6.009)))
	 (ampenv1 (make-env (list 0 0 .005 1 (- dur .008) 1 dur 0) :duration dur))
	 (ampenv2 (make-env (list 0 0 .005 1 dur 0) :duration dur))
	 (g0 (* .25 .75 amp))
	 (g1 (* .25 .75 amp))
	 (g2 (* .5 amp))
	 (g3 (* .5 .75 amp))
	 (st (seconds->samples beg))
	 (nd (+ st (seconds->samples dur))))
    (run
     (do ((i st (+ i 1)))
	 ((= i nd))
       (outa i (+ (* (env ampenv1)
		     (+ (* g0 (oscil osc0))
			(* g1 (oscil osc1))
			(* g2 (oscil osc2))))
		  (* (env ampenv2) g3 (oscil osc3))))))))


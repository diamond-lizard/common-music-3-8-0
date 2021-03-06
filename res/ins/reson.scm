(definstrument (reson startime dur pitch amp numformants indxfun skewfun pcskew skewat skewdc
		      vibfreq vibpc ranvibfreq ranvibpc degree distance reverb-amount data)
  ;; data is a list of lists of form '(ampf resonfrq resonamp ampat ampdc dev0 dev1 indxat indxdc)
  (let* ((beg (seconds->samples startime))
	 (end (+ beg (seconds->samples dur)))
	 (carriers (make-vector numformants))
	 (modulator (make-oscil pitch))
	 (ampfs (make-vector numformants))
	 (indfs (make-vector numformants))
	 (c-rats (make-vector numformants))
	 (frqf (make-env (stretch-envelope skewfun 25 (* 100 (/ skewat dur)) 75 (- 100 (* 100 (/ skewdc dur))))
			 :scaler (hz->radians (* pcskew pitch)) :duration dur))
	 (totalamp 0.0)
	 (loc (make-locsig degree distance reverb-amount))
	 (pervib (make-triangle-wave :frequency vibfreq
				     :amplitude (hz->radians (* vibpc pitch))))
	 (ranvib (make-rand-interp :frequency ranvibfreq
				   :amplitude (hz->radians (* ranvibpc pitch)))))
    ;; initialize the "formant" generators
    (do ((i 0 (+ i 1)))
	((= i numformants))
      (set! totalamp (+ totalamp ((data i) 2))))
    (do ((i 0 (+ i 1)))
	((= i numformants))
      (let* ((frmdat (data i))
	     (freq (cadr frmdat))
	     (ampf (car frmdat))
	     (rfamp  (frmdat 2))
	     (ampat (* 100 (/ (frmdat 3) dur)))
	     (ampdc (- 100 (* 100 (/ (frmdat 4) dur))))
	     (dev0 (hz->radians (* (frmdat 5) freq)))
	     (dev1 (hz->radians (* (frmdat 6) freq)))
	     (indxat (* 100 (/ (frmdat 7) dur)))
	     (indxdc (- 100 (* 100 (/ (frmdat 8) dur))))
	     (harm (round (/ freq pitch)))
	     (rsamp (- 1.0 (abs (- harm (/ freq pitch)))))
	     (cfq (* pitch harm)))
	(if (zero? ampat) (set! ampat 25))
	(if (zero? ampdc) (set! ampdc 75))
	(if (zero? indxat) (set! indxat 25))
	(if (zero? indxdc) (set! indxdc 75))
	(set! (indfs i) (make-env (stretch-envelope indxfun 25 indxat 75 indxdc) :duration dur
				       :scaler (- dev1 dev0) :offset dev0))
	(set! (ampfs i) (make-env (stretch-envelope ampf 25 ampat 75 ampdc) :duration dur
				       :scaler (* rsamp amp (/ rfamp totalamp))))
	(set! (c-rats i) harm)
	(set! (carriers i) (make-oscil cfq))))
    (run
     (do ((i beg (+ i 1)))
	 ((= i end))
       (let* ((outsum 0.0)
	      (vib (+ (triangle-wave pervib) (rand-interp ranvib) (env frqf)))
	      (modsig (oscil modulator vib)))
	 (do ((k 0 (+ 1 k)))
	     ((= k numformants))
	   (set! outsum (+ outsum
			   (* (env (ampfs k))
			      (oscil (carriers k) 
				     (+ (* vib (c-rats k))
					(* (env (indfs k)) modsig)))))))
	 (locsig loc i outsum))))))

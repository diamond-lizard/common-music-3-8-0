(definstrument (touch-tone start telephone-number)
  (let ((touch-tab-1 '(0 697 697 697 770 770 770 852 852 852 941 941 941))
	(touch-tab-2 '(0 1209 1336 1477 1209 1336 1477 1209 1336 1477 1209 1336 1477)))
    (do ((i 0 (+ i 1)))
	((= i (length telephone-number)))
      (let* ((k (telephone-number i))
	     (beg (seconds->samples (+ start (* i .4))))
	     (end (+ beg (seconds->samples .3)))
	     (i (if (number? k)
		    (if (not (= 0 k))
			k 
			11)
		    (if (eq? k '*) 
			10
			12)))
	     (frq1 (make-oscil (touch-tab-1 i)))
	     (frq2 (make-oscil (touch-tab-2 i))))
	(run
	 (do ((j beg (+ 1 j)))
	     ((= j end))
	   (outa j (* 0.1 (+ (oscil frq1) (oscil frq2))))))))))

;;; (with-sound () (touch-tone 0.0 '(7 2 3 4 9 7 1))
;;; I think the dial tone is 350 + 440
;;; http://www.hackfaq.org/telephony/telephone-tone-frequencies.shtml


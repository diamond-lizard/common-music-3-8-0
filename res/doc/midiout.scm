; This tutorial explains how to send data to the MIDI output port.
; Before you get started select a MIDI output device from the Midi Out
; menu and then test it using Test Output.  If no output devices are
; listed then you need to configure your computer for Midi playback.

; You can use code to open outout devices.  First let's list the
; available ports and their device names/numbers:

(ports)

; now open output device 0 (or whatever) on the midi out port

(send "mp:open" :out 0)

; Assuming you have a Midi Out device selected and you know that it
; makes sound, here is how to sound a Middle C (key 60) for .5 second.
; Put the cursor at the end of the command line and press
; COMMAND-Enter to listen to it:

(send "mp:midi" 0 .5 60 .5 0)

; In this next example we'll send random instrument assignments each
; time we send a sound and we'll pick only black keys to play the
; sounds

(let* ((someC (* 12 (between 2 9)))
       (black (+ someC (pick 1 3 6 8 10))))
  (send "mp:prog" :val (random 16))
  (send "mp:midi" :key black))

; to learn more about Program Changes (MIDI instrument assignments)
; see: http://www.midi.org/about-midi/gm/gm1sound.shtml

; how to send chords...

(let* ((inst (between 0 65))
       (kord (transpose (pick '((0 4 7)
                                (0 3 7 10)
                                (0 4 7 10)
                                (0 4 7 11)))
                        (between 48 80))))
  (send "mp:prog" :val inst)
  (loop for k in kord do
        (send "mp:midi" :key k)))

       

; now set the default instrument back to Grand Piano:

(send "mp:prog" :val 0)

;
;; About the 'send' command
;

; The  send command  sends data  to  ports.  The  string "mp:midi"  is
; called a 'message' -- it tells send where to route your data. Inside
; the message string  the name before the colon says  what port to use
; and  the name  after  the colon  specifies  the method  you want  to
; trigger on  that port.  So the  message "mp:midi" means  you want to
; send  your data to  the Midi  Port's 'midi'  method and  the message
; "mp:prog" says to send the  data to the prog (program change) method
; of the port.

; Following the message comes the comma delimited parameter data you
; want to send.  The "mp:midi" message allows up to five parameters to
; be specified.  The first parameter is named 'time', it holds the
; time stamp of the note, in seconds, where 0 means NOW! The second
; parameter is 'dur', the duration of the note in seconds. The third
; value is the 'key' parameter, it holds the MIDI key number of the
; note. The fourth value is 'amp', usually a value between zero and
; one where .5 means mezzo-forte. The last parameters is 'chan', the
; MIDI channel for the note, an integer from 0 to 15 (chan numbers are
; zero based!)

; Parameters for most messages have default values, which means that
; if you don't specify a value the the method will use a value it
; thinks is reasonable.  For example, you can make the exact same
; sound as the first example on your device by executing this:

(send "mp:midi")

; In other words, the default value for 'time' is 0, 'dur' is .5
; seconds, 'key' is 60, 'amp' is .5 and 'chan' is 0. For example to
; make the default sound but starting 2 second in the future and
; lasting for 3 seconds execute this:

(send "mp:midi" 2 3)

; If you just want to change just one or two parameters from their
; defaults consider using keyword names of the parameters. Named
; parameters can appear in any order:

(send "mp:midi" :amp .9 :key 71)

; note that in the case of named parameters the PAIRS,
; the first item in the pair is the keyword name of the parameter (the
; word starting with a colon) and the second item
; is the value you want
; to pass to it. Of course, parameter values can be expressions, not
; just values. For example, each time you execute the next example it
; sends a randomly chosen key between 50 and 80 that lasts either .1,
; .5 or 2 seconds:

(send "mp:midi" :key (between 50 80) :dur (pick .1 .5 2))

; you can send messages in the future simply providing the appropriate
; (future time stamp). For example this loop sends 8 messages, all but
; the first are sent in the future:

(loop repeat 8
      for t from 0 by .125
      do
      (send "mp:midi" t :key (between 4 90)))

; Try executing the expression several times allowing overlap (or not)
; beween successive gestures!

;
;; Microtonal Output
; 
; In order to make microtonal sound using the MIDI port you need to do
; two things: (1) Set your Midi Out port to a micotonal resolution and
; (2) Send floating point key numbers.

; You can use the Ports>Midi Out>Microtones> submenu to set your port
; to a Microtonal resoution your choice (see Help>Ports for more
; information about this) or use the "mp:tuning" message in a send
; expression. Th value you send to tuning is the number of divisions
; per semitones, so 2 puts the port into Quarter tone tuning:

(send "mp:tuning" 2)

; Once you have "tuned" your port to a microtonal resolution you can
; generate microtonal output simply by sending floating point key
; numbers in your data. Recall that Common Music interpets the
; floating point key number kkk.cc as the frequency that is cc cents
; above the key number kkk. So 60.5 means one quarter-tone above middle C:

(send "mp:midi" :key 60)

(send "mp:midi" :key 60.5)

(send "mp:midi" :key 61)

; Realize that the floating point key values you send are always
; quantized to the specific microtonal resolution that you set in your
; port:

(loop for i from 0.0 to 1.0 by .1
      for j from 0 by .5
      do
      (send "mp:midi" :time j :key (+ 60 i)))

; now divide semitones into 12 parts and try the loop again, since 12
; quantizes to 8 cent, you hear much better resolution of the 10 cent
; steps in the data:

(send "mp:tuning" 12)

(loop for i from 0.0 to 1.0 by .1
      for j from 0 by .5
      do
      (send "mp:midi" :time j :key (+ 60 i)))

; here s a little loop that generates the harmonics series. it
; converts a fundamenal key into hz, multiplies it by the harmonic,
; and converts it back into a floating point key number for midi to
; play!

(loop with fund = (hz 36)
      for harm from 1 to 16
      for time from 0 by .5
      do
      (send "mp:midi" :time time :key (key (* fund harm))))

; now let's set the port back to semitone tuning:

(send "mp:tuning" 1)

; define a process

(define (simp n r lb ub)
  (process repeat n
           do
           (send "mp:midi" :key (between lb ub))
           (wait r)))
    

;; sprout process to run in real time

(sprout (simp 10 .2 48 84))

; steve reich piano phase

(define (piano-phase endtime keys rate)
  (process with pat = (make-cycle keys)
           while (< (elapsed) endtime)
           do
           (send "mp:midi" :key (next pat) :dur rate)
           (wait rate)))

(let ((mynotes '(e4 fs4 b4 cs5 d5 fs4 e4 cs5 b4 fs4 d5 cs5))
      (endtime 20))
  (sprout (piano-phase endtime (key mynotes) .167))
  (sprout (piano-phase endtime (key mynotes) .170)))

; sprout process to a file.when you do this the scheduler runs file
; output faster than real time.

(sprout (simp 10 .25 60 72) "test.mid")


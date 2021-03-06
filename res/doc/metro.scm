;;; -*- syntax: Lisp; font-size: 18; theme: "Emacs"; -*-

;
;; Metronomes  (Halim Beere, halimbeere@gmail.com)
;

; Metronomes are objects that allow you to change the tempo of running
; processes in real-time. at least one metronome is always active,
; even if you do not specify a metronome the 'default metronome' will
; be in use.  to access a metronome you use its 'id', the metronom id
; of the default metronome is always 0:

; current beat time of the default metro

(metro-beat 0)

; most metronome functions take an optional 'metro' arg that lets you
; specify which metronome should be use. if you don't specify this
; value explicitly you will get whatever metronome is in the global
; varible *metro*. This variable is intiaily set to 0 (the default
; metronome) but you can reset it to any metronome that you create.

; if no metro is specified the metronome in *metro* will be used:

(metro-beat )

; use 'make-metro' to create a new metronome with some initial
; starting tempo. make-metro returns the new metronome's id as its
; value that you save in a variable for referenceing that metro

(define mymetro (make-metro 100))

; you can ask if there is a metro with a given id:

(metro? mymetro)

(metro? 0)

; you can get the current tempo and beat value of your metronome

(metro-tempo mymetro)

; get the tempo of the default metronome:

(metro-tempo )

; you can get a list of all the ids of all the metronomes

(metros )
; if you dont want to include the default metro in the list 

(metros #t)

; the default will always be the first in the list so you could also
; do

(rest (metros ))

; you can delete a metronome when you are done with it

(delete-metro mymetro)

(metro? mymetro)

(metros )

;
;; Using Metronomes With Processes 
;

; let's first create a process of running sixteenths, i.e.  it waits
; one-quarter of a beat after playing each note:

(define (dvorak )
  (process 
    with mel = (key '(a4 bf4 a4 g4 f4 e4 d4 bf2 d3 f4 e4 f4 d4 g2 d3 
                      f4 e4 d4 e4 a2 e4 f4 e4 f4 d4 d3 a3 d4 f4 g4))
    with pat = (make-cycle (concat mel mel (plus 12 mel) (plus 12 mel))) 
    with dur
    for k = (next pat)
    do
    (set! dur (if (< k 62) 1 1/4))
    (send "mp:midi" :key k :dur dur)
    (wait 1/4)))

    

; sprout the process to start it playing

(sprout (dvorak))

; the 'metro' function adjusts the tempo of a metronome in real-time:
; metro(tempo,time,metro) the frist arg is the new bpm value for the
; metromome, the second arg is the number of seconds for the metronome
; to move from its current tempo to the new tempo.  the optional third
; arg is the metronome to use, and defaults to the default metronome.

; lets move the dvorak process from bpm 60 to bpm 90 over 3 seconds:

(metro 90 3)

; now move to bpm 400 in 6 seconds

(metro 400 6)

; move back to 60 immediately

(metro 60 0)

; if a time is not specified, the default is to move the tempo in 0
; seconds. the following will move to bpm 40 immediately. you can
; also specify seconds with the keyword secs:

(metro 40)

(metro 40 :secs 0)

; you can also specify moving to a new tempo over a certain number
; of beats. that is, a linear tempo change will occur such that 10
; beats later we will arrive at the new tempo.

(metro 90 :beats 10)

; one can sprout a process as many times as one likes, and 
; each one will be independent from the others.  sprout
; piano-phase() again, and even though the speed of the process is
; dictated by the metronome tempo, the processes will not be aligned.

; execute the next line several times

(sprout (dvorak ))

; setting the tempo will still affect all running processes

(metro 10 :secs 4)

(metro 90)

; stop all processes

(stop )




;
;; Syncing Processes With Metronomes
;

; when a process is sprouted, it begins playing at the moment sprout()
; is evaluated. however, this will likely be at some fraction of the
; beat. the same happens when we ask for the current beat. the
; liklihood that we will get a round, integer number is slim. Try it:

(metro-beat )

; You got something like 888.74775660822. That would mean that you
; were presently at the 888th beat, and almost 3/4's of the way 
; through it. if you had sprouted a process then, it would
; have started at that point.

; in order to ensure that a process starts on a beat, you need to 
; pass a special function as the ahead value to the sprout. recall
; that sprout has an ahead parameter, which delays the sprouting of
; the process until the indicated time elapses.

; first, let's make a new process

(define (downbeats )
  (process
    with Cm = (make-heap '(c4 ef4 g4))
    with chrom = (make-heap '(cs4 d4 e4 f4 fs4 gs4 a4 as4 b4) :for 1)
    with pat = (make-cycle (list Cm chrom))
    with dur
    for k = (key (next pat))
    do
    (if (odds .1)
      (begin (set! dur 3)
             (send "mp:midi" :key (- k 12) :dur dur))
      (begin (set! dur 1)
             (send "mp:midi" :key (+ k (pick 0 12)) :dur dur)))
    (wait dur)))

; now let's sprout our new process 2 beats in the future.
; however, it you execute the below at beat 232.7, then the process
; will start at 234.7!  

(sprout (downbeats) 2)

(stop )

; to ensure that the process starts on a downbeat, pass in sync()
; as your ahead value. This way the process will start on a round
; beat number, such as 235.0.

(sprout (downbeats ) (sync ))

; sprout it again, and you will see that both processes are aligned
; with the downbeat.

(sprout (downbeats ) (sync ))

; sync() itself can take an ahead argument.  The default value is 1,
; which means the process will start at the NEXT downbeat.  The above
; sprouts are equivalent to:

(sprout (downbeats ) (sync 1))

; if you give sync() a value of .5, it will start your process on the
; nearest offbeat.  If you call sprout() at beat 102.23 for example,
; then your process will start at 102.5.  Calling sprout() at 102.59
; with sync(.5) will start your process at 103.5.

(sprout (downbeats ) (sync 0.5))

; give sync() a value of 1.75, and it will wait until the next beat
; arrives, then it will start your process at .75 of that beat.

(sprout (downbeats ) (sync 1.75))

; stop the above processes

(stop )

; now sync them to a different metronome

(define mymetro (make-metro 100))

(sprout (downbeats ) (sync :metro mymetro))

(sprout (downbeats ) (sync 1/3 :metro mymetro))

(sprout (downbeats ) (sync 2/3 :metro mymetro))

; stop all processes.

(stop )



;
;; Accouting For Durations With Metronomes
;

; While attack times for all events remain accurate, even while
; changing tempos, the duration values when sending midi data
; are in absolute seconds, not beats.

; Notice that even though the downbeats() process is waiting to play 
; every beat (at bpm 120), the durations are longer than they should 
; be, because "dur: 1" refers to seconds, not beats.

(metro 120)

(sprout (downbeats ) (sync ))

(stop )

; here is a new process. notice we have the same problem.
; even though our durations and wait values are the same, the dur:
; is in seconds, while wait is in beats.

(define (pathetic )
  (process 
    with mel-pat = (make-cycle 
                    (key '(b3 cs4 d cs b3 e4 d b3 d4 cs fs e e e 
                              g fs fs fs b3 as as as cs4 b3 b b fs e e e g
                              fs fs e d e d cs b2 as fs fs3)))
    with rhy-pat = (make-cycle '(.5 .5 1 2 .25 .25 .25 .25 1 2
                                    .25 .25 .25 .25 .25 .25 .25 .25
                                    .25 .25 .25 .25 .25 .25 .25 .25
                                    .25 .25 .25 .25 .25 .25 .25 .25
                                    .25 .25 .25 .25 .25 .25 .25 .25))
    for k = (next mel-pat)
    for d = (next rhy-pat)
    do
    (send "mp:midi" :key k :dur d)
    (wait d)))


; sprout it and notice how long the notes sustain.

(sprout (pathetic ) (sync ))

(stop )

; To correct for this we will use metro-dur() when indicating
; the durations of midi notes.

(define* (pathetic (m *metro*) )
  (process 
    with mel-pat = (make-cycle 
                    (key '(b3 cs4 d cs b3 e4 d b3 d4 cs fs e e e 
                              g fs fs fs b3 as as as cs4 b3 b b fs e e e g
                              fs fs e d e d cs b2 as fs fs3)))
    with rhy-pat = (make-cycle '(.5 .5 1 2 .25 .25 .25 .25 1 2
                                    .25 .25 .25 .25 .25 .25 .25 .25
                                    .25 .25 .25 .25 .25 .25 .25 .25
                                    .25 .25 .25 .25 .25 .25 .25 .25
                                    .25 .25 .25 .25 .25 .25 .25 .25))
    for k = (next mel-pat)
    for d = (next rhy-pat)
    ;for m = (make-metro m)
    do
    (send "mp:midi" :key k :dur (metro-dur 0.5 m))
    (wait d)))

; now try sprouting it. Notice this corrects the issue.

(sprout (pathetic ) (sync ))

; Even if we make the tempo faster, the durations will become shorter
; to accomodate the tempo.

(metro 200)

; or longer durations at a slower tempo

(metro 20 4)

; metro-dur() simply returns the amount of time in seconds that it will
; take for a beat amount under the current metronome. at bpm = 20,
; one beat should take 3 seconds.

(metro-dur 1)

; of course, you can pass in a custom metronome

(metro-dur 1 mymetro)


(stop )
; the same adjustments must be made if you take advantage of the
; "time" ahead parameter when sending a midi event.  Recall . . .
; this will trigger a midi event immediately.

(send "mp:midi" :key 60)

; and this will trigger it 1 second from now

(send "mp:midi" :time 1 :key 60)

; Thus, if you use the "time" midi parameter inside of a process,
; it will only work as expected if your metronome is at bpm = 60.
; This is because this is the tempo at which 1 beat is equal to 1
; second, and "time" (for mp:midi) is in seconds, not beats.

; here is a process that uses the "time" argument. Define it below.

(define (time-user )
  (process 
    with pat = (make-cycle '(2 1 .5 .25 .25))
    for rhy = (next pat)
    for off = (pick '(.25 .5 .75))
    do
    (send "mp:midi" :key (- 60 24) :dur rhy)
    (send "mp:midi" :key (- (pick '(67 68 70 72)) 12) :dur rhy)
    (send "mp:midi" :time off :key 63 :dur 1)
    (send "mp:midi" :time off :key (pick '(67 68 70 72)) :dur 1)
    (wait rhy)))


; first set the tempo to bpm = 60, then sprout it.  It should sound
; as expected.

(metro 60)

(sprout (time-user ) (sync ))

; however, if we change the tempo to bpm = 90, the rhythm sounds off.
; This is because of the issue described above.

(metro 90)

; set the tempo back to 60

(metro 60)

(stop )

; let's redefine our process to use metro-dur() and fix this

(define* (time-user (m *metro*))
  (process 
    with pat = (make-cycle '(2 1 .5 .25 .25))
    for rhy = (next pat)
    for off = (pick '(.25 .5 .75))
    do
    (send "mp:midi" :key (- 60 24) :dur (metro-dur rhy m))
    (send "mp:midi" :key (- (pick '(67 68 70 72)) 12) :dur (metro-dur rhy m))
    (send "mp:midi" :time off :key 63 :dur (metro-dur 1))
    (send "mp:midi" :time off :key (pick '(67 68 70 72)) :dur (metro-dur 1))
    (wait rhy)))

; try sprouting now

(sprout (time-user ) (sync ))
; and change the tempo

(metro 90)

(metro 40 5)

(stop )

;
;; Using Multiple Metronomes
;

; let's use two different processes while using two different metros
; we'll redefine our dvorak() process with a few slight adjustments,
; including the use of metro-dur()

(define* (dvorak (m *metro*))
  (process 
    with mel = (plus (key '(a4 bf4 a4 g4 f4 e4 d4 bf2 d3 f4 e4 f4 d4 g2 d3 
                               f4 e4 d4 e4 a2 e4 f4 e4 f4 d4 d3 a3 d4 f4 g4)) 9)
    with pat = (make-cycle (concat mel mel (plus 12 mel) (plus 12 mel))) 
    with dur
    for k = (next pat)
    do
    (send "mp:midi" :key k :dur (metro-dur 1/4 m))
    (wait 1/4)))


; Check that we already have a custom metronome existing

(metro? mymetro)
; or

(metros )

; set the default metro and sprout our pathetic() process

(metro 80)

(sprout (pathetic ) (sync ))

; now, set mymetro to the same tempo

(metro 80 :metro mymetro)

; sprout our dvorak() process using this metronome

(sprout (dvorak mymetro) (sync :metro mymetro))

; notice that the two metronomes are not aligned.  We created them at
; different times, and set their tempos at different times, so we
; can't expect them to be in sync.

; we can SYNC these two metronomes by calling metro-sync().

(metro-sync mymetro)

; metro-sync() takes a metronome that will be pushed or pulled in order
; to align a beat with another master-metronome. the master-metro is
; by default the global *metro*.

; force the metronomes out of sync for a moment.

(metro 20 :metro mymetro)

(metro 80 :metro mymetro)

; now we'll sync them again.

(metro-sync mymetro 10 :master-metro *metro*)

; above, you can see how I indicated the master-metro. if you want to
; sync to a metro other than the global, this is how you indicate it.
; the second number (10) means that the two metronomes will complete
; the sync 10 beats later (10 beats according to the master-metro).

;
;; Change Tempo while maintaining alignment
;

; Notice that changing the tempo normally will cause the
; metronome to go out of alignment with the other metro.  See here:
.

(metro 40 2 :metro mymetro)

; bring the tempo back up

(metro 80 2 :metro mymetro)

; If we change the tempo using the "tempo:" keyword in metro-sync(),
; we can prevent the metronomes from going out of alignment. This will
; guarantee that by the time mymetro has reached its goal tempo, its
; beats will still align with the master metronome. this slowdown will
; take place over 5 beats

(metro-sync mymetro 5 :tempo 40 :master-metro *metro*)

; now we can speed up mymetro back to normal tempo and still
; maintain alignment with the other metro. This speed up take
; place over 2 beats.

(metro-sync mymetro 2 :tempo 80)

; note that if we want to alter the global metronome, we should
; indicate what the master metronome will be.  If we do NOT indicate
; the master metronome, metro-sync() will automatically choose the
; first created user metronome as master. While this may seem strange,
; if we only have two metronomes (the user metronome, and the global),
; then it will correctly choose the only existing user metro.
; notice:

(metro-sync *metro* :tempo 40)

; Note that the transistion time for the above tempo change and sync
; was over the course of 1 beat.  1 is the default value if none is
; specified.

; But to be clear, you will probably want to explicitly indicate
; the master-metro if *metro* is the one to be altered.

(metro-sync *metro* :tempo 80 :master-metro mymetro)

; beats can be indicated by keyword

(metro-sync *metro* :beats 4 :tempo 40)

; one can indicate seconds instead, if desired

(metro-sync *metro* :secs 2 :tempo 80)

; metro-sync makes no presumption that the tempos of the two
; metronomes are equal, or are related by being twice as fast or slow.
; The tempos can be completely unrelated.  all that this function
; ensures is that after a certain number of beats, there will be
; a coinciding beat between the two metros.  

; for example, we know that the global metro is at bpm = 80.  if
; we set mymetro to bpm = 80 / 3, and sync them over 1 beat, then
; 1 beat later BOTH metros will align AT LEAST for that beat, then
; they will continue on with their respective tempos.

(metro-sync mymetro :tempo (/ 80 3))

; but of course, once these metros align, they are in a 1 to 3
; relationship.  Let's try another tempo ratio:

(metro-sync mymetro :tempo (* 80 2/3))

(stop )

; Note that metro-sync(), as well as metro-phase() explained below,
; both return #t when evaluated, as displayed in your console
; window. Both functions will return #f if they have decided
; to do nothing. This will happen if you try to execute phases
; or syncs rapidly in succession. trying to sync two metronomes
; before a previous sync was completed will result in no action
; being taken, and #f being returned by the function.

; #f will also be return by metro-sync() if the metronomes are
; already in sync and no adjustment is necessary.



;
;; Playing Reich's Piano Phase Manually
;

; let's define Reich's piano-phase to take advantage of metro-dur().
; we'll also pass in the metronome we're using as an argument
; so that metro-dur() is using the correct metronome

(define* (piano-phase (m *metro*)) ;(&optkey m = *metro*)
  (process 
    with pat = (make-cycle '(e4 fs4 b4 cs5 d5 fs4 e4 cs5 b4 fs4 d5 cs5))
    for k = (key (next pat))
    do
    (send "mp:midi" :key k :dur (metro-dur 1/4 m))
    (wait 1/4)))


; first, let's start the processes at the same tempo, and in sync.

(metro 100)

(metro-sync mymetro :tempo 100)

(begin
  (sprout (piano-phase ) (sync ))
  (sprout (piano-phase mymetro) (sync :metro mymetro)))


; one can play Reich's piano phase manually by using
; metro-phase().  metro-phase() takes two beat values -> the second
; value is the number of beats that would normally occur. this is the
; space into which you will fit a different number of beats as
; indicated by the first value.

; for example.  metro-phase(5.25, 5, *metro*), means to take the
; global metronome and over what would normally be the next 5 beats,
; speed up the tempo slightly so that 5.25 beats occur instead.

(metro-phase 5.25 5)

; *metro* is the default metronome, as usual.
; Here we cause the metronome to slow down slightly, so that fewer
; beats occur (4.75) in the space of 5 beats.

(metro-phase 4.75 5)

; this transition take twice as long, and is more gradual.
(metro-phase 10.25 10 mymetro)

; this transition is rather sudden
(metro-phase 1.25 1 :metro mymetro)

; of course, you can "phase" by any amount you want
(metro-phase 32 30 :metro mymetro)

; remember to watch your console window. if you start a phase that
; will take a long time, and you try to phase before the first phase
; is completed, the console window will display #f -> false.

(stop )

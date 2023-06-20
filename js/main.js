let board = null;
let game = new Chess();

let pieceVal = {"B": 3, "N": 3, "R": 5, "Q": 9}

function makeRandomMove() {
  let possibleMoves = game.moves();

  if (game.game_over()) {
    console.log(game.pgn());
    return;
  }

  let captureMove = "-1";
  let captureMoveVal = 0;

  possibleMoves.forEach((move) => {
    if (move[move.length - 3] == "x") {
      captureMove = move;
      let val = 1;
      for (let i = 0; i < move.length; i++) {
        if (move[i] == move[i].toUpperCase()) {
          val = pieceVal[move[i]];
          break;
        }
      }
      if (captureMoveVal < val) {
        captureMoveVal = val;
      }
    }
  });

  console.log(captureMove);

  if (captureMove != "-1") {
    game.move(captureMove);
  } else {
    let randomIdx = Math.floor(Math.random() * possibleMoves.length);
    game.move(possibleMoves[randomIdx]);
  }

  board.position(game.fen());
}

function onDragStart (source, piece, position, orientation) {
  if (game.game_over()) return false

  if (piece.search(/^b/) !== -1) return false
}

function onDrop (source, target) {
  var move = game.move({
    from: source,
    to: target,
    promotion: 'q' // NOTE: always promote to a queen for simplicity
  })

  if (move === null) return 'snapback'

  // make random legal move for black
  window.setTimeout(makeRandomMove, 250)
}

// update the board position after the piece snap
// for castling, en peasant, pawn promotion
function onSnapEnd () {
  board.position(game.fen())
}

var config = {
  draggable: true,
  position: 'start',
  onDragStart: onDragStart,
  onDrop: onDrop,
  onSnapEnd: onSnapEnd
}

board = Chessboard("myBoard", config);
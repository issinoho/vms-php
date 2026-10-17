<?php
// r_fc.php: opcache.file_cache round trip (Phase 3). Run once to write the
// .bin, change VERSION, run again with opcache.file_cache_only=1 and
// validate_timestamps=0: the old VERSION means the script came from the file
// cache, and the checksum must match the first run's.
const VERSION = 'one';
enum Suit: string { case Hearts = 'H'; case Spades = 'S'; }
interface Shape { public function area(): float; }
abstract class Base implements Shape {
    public static int $made = 0;
    public function __construct(protected string $name) { static::$made++; }
    abstract public function area(): float;
    public function __toString(): string { return sprintf('%s(%.3f)', $this->name, $this->area()); }
}
final class Circle extends Base {
    public function __construct(private float $r) { parent::__construct('circle'); }
    public function area(): float { return M_PI * $this->r ** 2; }
}
final class Rect extends Base {
    public function __construct(private float $w, private float $h) { parent::__construct('rect'); }
    public function area(): float { return $this->w * $this->h; }
}
function gen(int $n): Generator { for ($i = 1; $i <= $n; $i++) yield $i => $i * $i; }
$shapes = [new Circle(1.5), new Rect(2, 3.25), new Circle(0.5)];
usort($shapes, fn(Shape $a, Shape $b) => $a->area() <=> $b->area());
$data = [
    'shapes' => array_map('strval', $shapes),
    'made' => Base::$made,
    'suits' => array_map(fn($s) => $s->name . '=' . $s->value, Suit::cases()),
    'squares' => iterator_to_array(gen(6)),
    'match' => match (true) { count($shapes) > 2 => 'many', default => 'few' },
    'heredoc' => <<<TXT
        static text with {$shapes[0]} inside
        TXT,
    'nested' => ['a' => [1, 2.5, null, true, 'x' => ['y' => 'z']]],
];
$out = json_encode($data);
echo VERSION, ' ', md5($out), ' ', strlen($out), "\n";

import argparse
from pathlib import Path
import shutil

import numpy as np
from PIL import Image
import torch
from torch import nn
from torchvision.models import resnet18, ResNet18_Weights


def main(img_path: Path, out_dir: Path):
    if out_dir.exists():
        assert all(p.suffix == '.jpg' for p in out_dir.glob('*'))
        shutil.rmtree(args.out_dir)

    out_dir.mkdir(parents=True)
    im = Image.open(img_path)
    weights = ResNet18_Weights.DEFAULT
    preprocess = weights.transforms()
    model = resnet18(weights=weights)
    model.eval()
    model = model.cuda()
    # import pdb; pdb.set_trace()
    x = preprocess(im)[None, ...].cuda()
    layer_counter = 0
    for name, layer in model.named_children():
        if name == 'fc':
            break
        with torch.inference_mode():
            x = layer(x)
        if (
            isinstance(layer, nn.Conv2d)
            or isinstance(layer, nn.MaxPool2d)
            or isinstance(layer, nn.Sequential)
            or isinstance(layer, nn.AdaptiveAvgPool2d)
        ):
            N, C, H, W = x.shape
            for c in range(0, C - 3, 3):
                img = x[0, c:c+3].permute((1, 2, 0)).cpu().numpy()
                q10 = np.quantile(img, 0.1)
                img -= q10
                q90 = np.quantile(img, 0.9)
                img /= q90
                img = np.clip(img, 0, 1)
                img = (img * 255).astype(np.uint8)
                Image.fromarray(img).save(
                    out_dir / f'{layer_counter:02}_{c:04}.jpg')
            layer_counter += 1


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('image', help='Image to pass through the network.')
    parser.add_argument('--out_dir', help='Output directory',
                        default=str(Path(__file__).parent / 'layer_outputs'))
    args = parser.parse_args()
    img_path = Path(args.image)
    out_dir = Path(args.out_dir)

    main(img_path, out_dir)
